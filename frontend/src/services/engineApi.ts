import type { EngineApi } from '@/types';
import { mockEngineApi } from './mockEngineApi';

export type { EngineApi };

/**
 * Single entry point for engine communication. Features depend on this export only,
 * so swapping the implementation requires no UI changes.
 *
 * TODO(V4): add `WebSocketEngineApi` (streaming `info` lines) and a REST client,
 * and select the implementation from `import.meta.env.VITE_ENGINE_MODE`.
 */
export const engineApi: EngineApi = mockEngineApi;
