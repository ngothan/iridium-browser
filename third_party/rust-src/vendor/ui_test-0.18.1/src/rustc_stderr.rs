use bstr::ByteSlice;
use regex::Regex;
use std::{
    num::NonZeroUsize,
    path::{Path, PathBuf},
};

#[derive(serde::Deserialize, Debug)]
struct RustcMessage {
    rendered: Option<String>,
    spans: Vec<RustcSpan>,
    level: String,
    message: String,
    children: Vec<RustcMessage>,
}

#[derive(Copy, Clone, Debug, PartialOrd, Ord, PartialEq, Eq)]
pub(crate) enum Level {
    Ice = 5,
    Error = 4,
    Warn = 3,
    Help = 2,
    Note = 1,
    /// Only used for "For more information about this error, try `rustc --explain EXXXX`".
    FailureNote = 0,
}

#[derive(Debug)]
/// A diagnostic message.
pub struct Message {
    pub(crate) level: Level,
    pub(crate) message: String,
    pub(crate) line_col: Option<Span>,
}

/// Information about macro expansion.
#[derive(serde::Deserialize, Debug)]
struct Expansion {
    span: RustcSpan,
}

#[derive(serde::Deserialize, Debug)]
struct RustcSpan {
    #[serde(flatten)]
    line_col: Span,
    file_name: PathBuf,
    is_primary: bool,
    expansion: Option<Box<Expansion>>,
}

#[derive(serde::Deserialize, Debug, Copy, Clone)]
pub struct Span {
    pub line_start: NonZeroUsize,
    pub column_start: NonZeroUsize,
    pub line_end: NonZeroUsize,
    pub column_end: NonZeroUsize,
}

impl Span {
    pub const INVALID: Self = Self {
        line_start: NonZeroUsize::MAX,
        column_start: NonZeroUsize::MAX,
        line_end: NonZeroUsize::MAX,
        column_end: NonZeroUsize::MAX,
    };

    pub fn shrink_to_end(self) -> Span {
        Self {
            line_start: self.line_end,
            column_start: self.column_end,
            ..self
        }
    }

    pub fn shrink_to_start(self) -> Span {
        Self {
            line_end: self.line_start,
            column_end: self.column_start,
            ..self
        }
    }
}

impl std::str::FromStr for Level {
    type Err = String;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "ERROR" | "error" => Ok(Self::Error),
            "WARN" | "warning" => Ok(Self::Warn),
            "HELP" | "help" => Ok(Self::Help),
            "NOTE" | "note" => Ok(Self::Note),
            "failure-note" => Ok(Self::FailureNote),
            "error: internal compiler error" => Ok(Self::Ice),
            _ => Err(format!("unknown level `{s}`")),
        }
    }
}

#[derive(Debug)]
pub(crate) struct Diagnostics {
    /// Rendered and concatenated version of all diagnostics.
    /// This is equivalent to non-json diagnostics.
    pub rendered: Vec<u8>,
    /// Per line, a list of messages for that line.
    pub messages: Vec<Vec<Message>>,
    /// Messages not on any line (usually because they are from libstd)
    pub messages_from_unknown_file_or_line: Vec<Message>,
}

impl RustcMessage {
    fn line(&self, file: &Path) -> Option<Span> {
        let span = |primary| self.spans.iter().find_map(|span| span.line(file, primary));
        span(true).or_else(|| span(false))
    }

    /// Put the message and its children into the line-indexed list.
    fn insert_recursive(
        self,
        file: &Path,
        messages: &mut Vec<Vec<Message>>,
        messages_from_unknown_file_or_line: &mut Vec<Message>,
        line: Option<Span>,
    ) {
        let line = self.line(file).or(line);
        let msg = Message {
            level: self.level.parse().unwrap(),
            message: self.message,
            line_col: line,
        };
        if let Some(line) = line {
            if messages.len() <= line.line_start.get() {
                messages.resize_with(line.line_start.get() + 1, Vec::new);
            }
            messages[line.line_start.get()].push(msg);
        // All other messages go into the general bin, unless they are specifically of the
        // "aborting due to X previous errors" variety, as we never want to match those. They
        // only count the number of errors and provide no useful information about the tests.
        } else if !(msg.message.starts_with("aborting due to")
            && msg.message.contains("previous error"))
        {
            messages_from_unknown_file_or_line.push(msg);
        }
        for child in self.children {
            child.insert_recursive(file, messages, messages_from_unknown_file_or_line, line)
        }
    }
}

impl RustcSpan {
    /// Returns the most expanded line number *in the given file*, if possible.
    fn line(&self, file: &Path, primary: bool) -> Option<Span> {
        if let Some(exp) = &self.expansion {
            if let Some(line) = exp.span.line(file, primary && !self.is_primary) {
                return Some(line);
            }
        }
        ((!primary || self.is_primary) && self.file_name == file).then_some(self.line_col)
    }
}

pub(crate) fn filter_annotations_from_rendered(rendered: &str) -> std::borrow::Cow<'_, str> {
    let annotations = Regex::new(r" *//(\[[a-z,]+\])?~.*").unwrap();
    annotations.replace_all(rendered, "")
}

pub(crate) fn process(file: &Path, stderr: &[u8]) -> Diagnostics {
    let mut rendered = Vec::new();
    let mut messages = vec![];
    let mut messages_from_unknown_file_or_line = vec![];
    for (line_number, line) in stderr.lines_with_terminator().enumerate() {
        if line.starts_with_str(b"{") {
            match serde_json::from_slice::<RustcMessage>(line) {
                Ok(msg) => {
                    rendered.extend(
                        filter_annotations_from_rendered(msg.rendered.as_ref().unwrap()).as_bytes(),
                    );
                    msg.insert_recursive(
                        file,
                        &mut messages,
                        &mut messages_from_unknown_file_or_line,
                        None,
                    );
                }
                Err(err) => {
                    panic!(
                        "failed to parse rustc JSON output at line {line_number}: {err}: {}",
                        line.to_str_lossy()
                    )
                }
            }
        } else {
            // FIXME: do we want to throw interpreter stderr into a separate file?
            rendered.extend(line);
        }
    }
    Diagnostics {
        rendered,
        messages,
        messages_from_unknown_file_or_line,
    }
}
