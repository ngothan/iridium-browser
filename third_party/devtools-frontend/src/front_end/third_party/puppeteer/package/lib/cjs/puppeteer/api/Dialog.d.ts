/**
 * Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import { type Protocol } from 'devtools-protocol';
/**
 * Dialog instances are dispatched by the {@link Page} via the `dialog` event.
 *
 * @remarks
 *
 * @example
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   page.on('dialog', async dialog => {
 *     console.log(dialog.message());
 *     await dialog.dismiss();
 *     await browser.close();
 *   });
 *   page.evaluate(() => alert('1'));
 * })();
 * ```
 *
 * @public
 */
export declare abstract class Dialog {
    #private;
    /**
     * @internal
     */
    constructor(type: Protocol.Page.DialogType, message: string, defaultValue?: string);
    /**
     * The type of the dialog.
     */
    type(): Protocol.Page.DialogType;
    /**
     * The message displayed in the dialog.
     */
    message(): string;
    /**
     * The default value of the prompt, or an empty string if the dialog
     * is not a `prompt`.
     */
    defaultValue(): string;
    /**
     * @internal
     */
    protected abstract sendCommand(options: {
        accept: boolean;
        text?: string;
    }): Promise<void>;
    /**
     * A promise that resolves when the dialog has been accepted.
     *
     * @param promptText - optional text that will be entered in the dialog
     * prompt. Has no effect if the dialog's type is not `prompt`.
     *
     */
    accept(promptText?: string): Promise<void>;
    /**
     * A promise which will resolve once the dialog has been dismissed
     */
    dismiss(): Promise<void>;
}
//# sourceMappingURL=Dialog.d.ts.map