// WebSocket 클라이언트 + 자동 mock 폴백.
//
// ws://127.0.0.1:8092 로 붙어보고 2초 안에 열리지 않으면 mock/sample-timeline.json 을
// 실시간처럼 재생한다. 덕분에 언리얼 없이 npm run dev 만으로 타임라인 화면을 개발할 수 있다.

import type { EventBatch, SessionHeader, TimelineEvent, TimelineSnapshot, WebDebugMessage } from './schema';

export type FeedMode = 'live' | 'mock' | 'file' | 'connecting';

export interface FeedState {
	mode: FeedMode;
	session: SessionHeader | null;
	events: TimelineEvent[];
	/** 지금까지 도착한 마지막 이벤트 시각 (초) */
	latestT: number;
	connectedAt: number | null;
}

type Listener = (s: FeedState) => void;

const WS_URL = `ws://${location.hostname === 'localhost' ? '127.0.0.1' : location.hostname}:8092`;
const CONNECT_TIMEOUT_MS = 2000;
const MOCK_TIMELINE_URL = '/sample-timeline.json';

export class TimelineFeed {
	private state: FeedState = { mode: 'connecting', session: null, events: [], latestT: 0, connectedAt: null };
	private listeners = new Set<Listener>();
	private socket: WebSocket | null = null;
	private mockTimer: number | null = null;
	private disposed = false;

	subscribe(fn: Listener): () => void {
		this.listeners.add(fn);
		fn(this.state);
		return () => this.listeners.delete(fn);
	}

	getState(): FeedState {
		return this.state;
	}

	start(): void {
		this.connectLive();
	}

	dispose(): void {
		this.disposed = true;
		this.socket?.close();
		this.socket = null;
		if (this.mockTimer !== null) window.clearInterval(this.mockTimer);
		this.mockTimer = null;
	}

	/** 스냅샷 JSON 파일을 그대로 올려 재생 없이 정지 상태로 본다 */
	loadSnapshot(snapshot: TimelineSnapshot): void {
		this.socket?.close();
		this.socket = null;
		if (this.mockTimer !== null) window.clearInterval(this.mockTimer);
		this.mockTimer = null;
		const events = [...snapshot.events].sort((a, b) => a.t - b.t);
		this.emit({
			mode: 'file',
			session: snapshot.session,
			events,
			latestT: events.length ? events[events.length - 1].t : 0,
			connectedAt: Date.now(),
		});
	}

	private emit(next: Partial<FeedState>): void {
		this.state = { ...this.state, ...next };
		for (const fn of this.listeners) fn(this.state);
	}

	private connectLive(): void {
		let settled = false;
		let socket: WebSocket;
		try {
			socket = new WebSocket(WS_URL);
		} catch {
			this.startMock();
			return;
		}
		// UE 의 libwebsockets 서버는 항상 바이너리 프레임으로 보낸다 — Blob 대신 ArrayBuffer 로 받아 디코딩한다
		socket.binaryType = 'arraybuffer';
		this.socket = socket;

		const timeout = window.setTimeout(() => {
			if (settled) return;
			settled = true;
			socket.close();
			this.socket = null;
			this.startMock();
		}, CONNECT_TIMEOUT_MS);

		socket.onopen = () => {
			settled = true;
			window.clearTimeout(timeout);
			this.emit({ mode: 'live', events: [], session: null, latestT: 0, connectedAt: Date.now() });
		};

		socket.onmessage = (ev) => {
			const text =
				typeof ev.data === 'string' ? ev.data : new TextDecoder('utf-8').decode(new Uint8Array(ev.data as ArrayBuffer));
			let msg: WebDebugMessage;
			try {
				msg = JSON.parse(text);
			} catch {
				return;
			}
			this.ingest(msg);
		};

		socket.onerror = () => {
			if (settled) return;
			settled = true;
			window.clearTimeout(timeout);
			this.socket = null;
			this.startMock();
		};

		socket.onclose = () => {
			if (settled) {
				// 연결이 살아 있다가 끊긴 경우 — 게임이 꺼진 것이므로 mock 으로 넘어간다
				if (this.state.mode === 'live' && !this.disposed) this.startMock();
				return;
			}
			settled = true;
			window.clearTimeout(timeout);
			this.startMock();
		};
	}

	private ingest(msg: WebDebugMessage): void {
		if (msg.kind === 'session') {
			// 새 전투가 시작되면 링 버퍼를 비우고 다시 쌓는다
			this.emit({ session: msg as SessionHeader, events: [], latestT: 0 });
			return;
		}
		if (msg.kind === 'events') {
			const batch = msg as EventBatch;
			if (!batch.events.length) return;
			const events = this.state.events.concat(batch.events);
			this.emit({ events, latestT: Math.max(this.state.latestT, batch.events[batch.events.length - 1].t) });
		}
	}

	/** mock/sample-timeline.json 을 15 Hz 로 실시간처럼 흘려보낸다 */
	private async startMock(): Promise<void> {
		if (this.disposed) return;
		this.emit({ mode: 'mock', events: [], session: null, latestT: 0 });
		let snapshot: TimelineSnapshot;
		try {
			const res = await fetch(MOCK_TIMELINE_URL);
			snapshot = await res.json();
		} catch {
			return;
		}
		if (this.disposed) return;

		const all = [...snapshot.events].sort((a, b) => a.t - b.t);
		const total = all.length ? all[all.length - 1].t : 0;
		this.emit({ session: snapshot.session, events: [], latestT: 0 });

		let cursor = 0;
		let playhead = 0;
		const STEP = 1 / 15;
		this.mockTimer = window.setInterval(() => {
			playhead += STEP;
			const batch: TimelineEvent[] = [];
			while (cursor < all.length && all[cursor].t <= playhead) batch.push(all[cursor++]);
			if (batch.length) {
				this.emit({ events: this.state.events.concat(batch), latestT: playhead });
			} else {
				this.emit({ latestT: playhead });
			}
			if (playhead > total + 1) {
				// 끝나면 처음부터 다시 재생 — 개발 중 계속 흐르는 화면이 필요하다
				cursor = 0;
				playhead = 0;
				this.emit({ events: [], latestT: 0 });
			}
		}, 1000 / 15);
	}
}

/* ────────────────────────── 런 로그 로딩 ────────────────────────── */

export interface RunListEntry {
	id: string;
	file: string;
}

/**
 * /api/runs 를 먼저 시도하고, 실패하면 mock/runs/index.json 으로 폴백한다.
 * 게임이 꺼져 있어도 /stats 가 동작해야 하므로 폴백은 필수다.
 */
export async function fetchRunList(): Promise<{ entries: RunListEntry[]; mock: boolean }> {
	try {
		const res = await fetch('/api/runs', { cache: 'no-store' });
		if (res.ok) {
			const data = await res.json();
			if (Array.isArray(data) && data.length) {
				return { entries: data.map(normalizeEntry), mock: false };
			}
		}
	} catch {
		/* 게임이 꺼져 있음 — mock 으로 간다 */
	}
	const res = await fetch('/runs/index.json', { cache: 'no-store' });
	const data = await res.json();
	return { entries: (data as RunListEntry[]).map(normalizeEntry), mock: true };
}

function normalizeEntry(raw: unknown): RunListEntry {
	const e = raw as Partial<RunListEntry> & { name?: string };
	const id = e.id ?? e.name ?? String(raw);
	return { id, file: e.file ?? `runs/${id}.json` };
}

export async function fetchRun(entry: RunListEntry, mock: boolean): Promise<unknown> {
	const url = mock ? `/${entry.file}` : `/api/runs/${entry.id}`;
	const res = await fetch(url, { cache: 'no-store' });
	if (!res.ok) throw new Error(`${url} → ${res.status}`);
	return res.json();
}
