/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/tabs/primary-tab';

import {html, LitElement} from 'lit';

/** A chromeOS compliant tab component for use in cros-tabs. */
export class Tab extends LitElement {
  /** @nocollapse */
  static override properties = {
    active: {type: Boolean, reflect: true},
    tabName: {type: String},
  };

  active: boolean;
  tabName: string;

  constructor() {
    super();
    this.active = false;
    this.tabName = '';
  }

  override firstUpdated() {
    this.setAttribute('md-tab', 'tab');
  }

  override render() {
    return html`
      <md-primary-tab ?active=${this.active}>
        ${this.tabName}
      </md-primary-tab>
    `;
  }
}

customElements.define('cros-tab', Tab);

declare global {
  interface HTMLElementTagNameMap {
    'cros-tab': Tab;
  }
}
