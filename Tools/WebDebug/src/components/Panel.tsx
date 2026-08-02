// 차트 한 장을 감싸는 공용 패널 — 제목, 해석 노트, 본문.
// 진입 애니메이션은 정적인 영역에만 붙인다(실시간 갱신 영역 금지).

import type { ReactNode } from 'react';

interface Props {
	title: string;
	/** 이 차트를 어떻게 읽어야 하는지 한 줄 — 튜닝 판단의 근거가 된다 */
	note?: string;
	aside?: ReactNode;
	children: ReactNode;
	animate?: boolean;
}

export default function Panel({ title, note, aside, children, animate = true }: Props) {
	return (
		<section className={`panel${animate ? ' enter' : ''}`}>
			<div className="panel__head">
				<h2 className="panel__title">{title}</h2>
				<span className="spacer" />
				{aside}
			</div>
			{note && <p className="panel__note">{note}</p>}
			{children}
		</section>
	);
}
