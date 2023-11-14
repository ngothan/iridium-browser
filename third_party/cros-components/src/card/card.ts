/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/ripple/ripple.js';
import '@material/web/focus/md-focus-ring.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

const DEFAULT_TICK_MARGIN = css`14px`;

function renderTick() {
  return html`
      <svg
          class="selected-tick"
          width="20"
          height="20"
          xmlns="http://www.w3.org/2000/svg">
        <path
            fill-rule="evenodd"
            clip-rule="evenodd"
            d="M19 10a9 9 0 1 1-18 0 9 9 0 0 1 18 0Zm-4.83-3.045a1.125 1.125 0 0
                0-1.59 0l-3.705 3.704L7.42 9.204a1.125 1.125 0 0 0-1.59
                1.591l2.25 2.25c.439.44 1.151.44 1.59
                0l4.5-4.5c.44-.439.44-1.151 0-1.59Z"/>
      </svg>`;
}

/** A chromeOS compliant card. */
export class Card extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      color: var(--cros-sys-on_surface);
      display: inline-block;
      font: var(--cros-body-0-font);
    }

    #container {
      align-items: center;
      background-color: var(--cros-card-background-color, var(--cros-sys-app_base));
      border-radius: 12px;
      display: grid;
      outline: 1px solid var(--cros-card-border-color, var(--cros-sys-separator));;
      padding: var(--cros-card-padding, 16px);
      position: relative;
    }

    :host([cardStyle="filled"]) #container {
      outline: none;
    }

    :host([cardStyle="elevated"]) #container {
      background-color: var(--cros-card-background-color, var(--cros-sys-base_elevated));
      box-shadow: var(--cros-card-elevation-shadow, var(--cros-elevation-1-shadow));
      outline: none;
    }

    :host([selected]) #container {
      background-color: var(--cros-card-selected-background-color, var(--cros-sys-primary_container));
    }

    md-ripple {
      color: var(--cros-sys-ripple_primary);
      --md-ripple-hover-color: var(--cros-card-hover-color, var(--cros-sys-hover_on_subtle));
      --md-ripple-hover-opacity: 1;
      --md-ripple-pressed-color: var(--cros-card-pressed-color, var(--cros-sys-ripple_neutral_on_subtle));
      --md-ripple-pressed-opacity: 1;
    }

    md-focus-ring {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-focus-ring-duration: 0s;
      --md-focus-ring-outward-offset: 0px;
      --md-focus-ring-shape: 12px;
    }

    :host([disabled]) {
      opacity: var(--cros-disabled-opacity);
    }

    .selected-tick {
      display: none;
    }

    :host([selected]) .selected-tick {
      bottom: var(--cros-card-icon-bottom, ${DEFAULT_TICK_MARGIN});
      display: unset;
      fill: var(--cros-card-icon-color, var(--cros-sys-on_primary_container));
      height: 20px;
      inset-inline-end: var(--cros-card-icon-inline-end, ${
      DEFAULT_TICK_MARGIN});
      position: absolute;
      width: 20px;
    }
  `;

  /** @nocollapse */
  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  /** @export */
  disabled = false;

  /** @export */
  selected = false;

  /** @export */
  cardStyle: 'outline'|'filled'|'elevated' = 'outline';

  /** @nocollapse */
  static override properties = {
    cardStyle: {type: String, reflect: true},
    disabled: {type: Boolean, reflect: true},
    selected: {type: Boolean, reflect: true},
    tabIndex: {type: Number},
  };

  override render() {
    return html`
        <div id="container" tabindex=${this.tabIndex}>
          <md-ripple
              for="container"
              ?disabled=${this.disabled}>
          </md-ripple>
          <md-focus-ring
              for="container"
              ?disabled=${this.disabled}>
          </md-focus-ring>
          ${renderTick()}
          <slot></slot>
        </div>
    `;
  }
}

customElements.define('cros-card', Card);

declare global {
  interface HTMLElementTagNameMap {
    'cros-card': Card;
  }
}
