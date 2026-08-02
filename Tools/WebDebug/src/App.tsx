import { useEffect, useMemo, useState } from 'react';
import { NavLink, Navigate, Route, Routes } from 'react-router-dom';
import Timeline from './routes/Timeline';
import RunStats from './routes/RunStats';
import { TimelineFeed, type FeedState } from './lib/ws';
import type { ThemeMode } from './lib/palette';

const MODE_LABEL: Record<FeedState['mode'], string> = {
	live: 'LIVE',
	mock: 'MOCK MODE',
	file: 'SNAPSHOT',
	connecting: 'CONNECTING…',
};

export default function App() {
	// 피드는 앱 수명 내내 하나만 둔다 — 라우트를 오가도 링 버퍼가 유지된다
	const feed = useMemo(() => new TimelineFeed(), []);
	const [state, setState] = useState<FeedState>(feed.getState());
	const [theme, setTheme] = useState<ThemeMode>('dark');

	useEffect(() => {
		const off = feed.subscribe(setState);
		feed.start();
		return () => {
			off();
			feed.dispose();
		};
	}, [feed]);

	useEffect(() => {
		document.documentElement.setAttribute('data-theme', theme);
	}, [theme]);

	return (
		<div className="app">
			<header className="topbar">
				<span className="brand">ASHEN CATHEDRAL · Web Debug</span>
				<nav className="nav">
					<NavLink to="/timeline" className={({ isActive }) => (isActive ? 'active' : '')}>
						전투 타임라인
					</NavLink>
					<NavLink to="/stats" className={({ isActive }) => (isActive ? 'active' : '')}>
						런 통계
					</NavLink>
				</nav>
				<span className="spacer" />
				<span className={`badge badge--${state.mode}`}>{MODE_LABEL[state.mode]}</span>
				<button onClick={() => setTheme(theme === 'dark' ? 'light' : 'dark')} title="테마 전환">
					{theme === 'dark' ? '◐ 다크' : '◑ 라이트'}
				</button>
			</header>

			<main className="content">
				<Routes>
					<Route path="/" element={<Navigate to="/timeline" replace />} />
					<Route path="/timeline" element={<Timeline feed={feed} state={state} theme={theme} />} />
					<Route path="/stats" element={<RunStats theme={theme} />} />
					<Route path="*" element={<Navigate to="/timeline" replace />} />
				</Routes>
			</main>
		</div>
	);
}
