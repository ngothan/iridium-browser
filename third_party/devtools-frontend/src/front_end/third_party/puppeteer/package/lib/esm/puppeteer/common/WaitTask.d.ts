/**
 * Copyright 2022 Google Inc. All rights reserved.
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
import { type ElementHandle } from '../api/ElementHandle.js';
import { type Realm } from '../api/Realm.js';
import { type HandleFor } from './types.js';
/**
 * @internal
 */
export interface WaitTaskOptions {
    polling: 'raf' | 'mutation' | number;
    root?: ElementHandle<Node>;
    timeout: number;
    signal?: AbortSignal;
}
/**
 * @internal
 */
export declare class WaitTask<T = unknown> {
    #private;
    constructor(world: Realm, options: WaitTaskOptions, fn: ((...args: unknown[]) => Promise<T>) | string, ...args: unknown[]);
    get result(): Promise<HandleFor<T>>;
    rerun(): Promise<void>;
    terminate(error?: Error): Promise<void>;
    /**
     * Not all errors lead to termination. They usually imply we need to rerun the task.
     */
    getBadError(error: unknown): Error | undefined;
}
/**
 * @internal
 */
export declare class TaskManager {
    #private;
    add(task: WaitTask<any>): void;
    delete(task: WaitTask<any>): void;
    terminateAll(error?: Error): void;
    rerunAll(): Promise<void>;
}
//# sourceMappingURL=WaitTask.d.ts.map