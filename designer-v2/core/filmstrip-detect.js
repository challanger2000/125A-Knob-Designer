const COMMON_FRAME_COUNTS = [2, 3, 4, 5, 6, 8, 10, 12, 16, 24, 32, 48, 64, 96, 100, 128, 192, 256];

function scoreCandidate(candidate) {
  const { frameWidth, frameHeight, frameCount, layout } = candidate;
  const aspectPenalty = Math.abs(Math.log(frameWidth / frameHeight));
  const commonBonus = COMMON_FRAME_COUNTS.includes(frameCount) ? 1.5 : 0;
  const squareBonus = Math.abs(frameWidth - frameHeight) <= 1 ? 2.0 : 0;
  const layoutBonus = layout === 'vertical' ? 0.35 : layout === 'horizontal' ? 0.25 : 0;
  const sizePenalty = frameWidth < 8 || frameHeight < 8 ? 8 : 0;
  return commonBonus + squareBonus + layoutBonus - aspectPenalty * 2.2 - sizePenalty;
}

function pushCandidate(list, candidate) {
  if (candidate.frameCount < 2 || candidate.frameCount > 256) return;
  if (candidate.frameWidth < 1 || candidate.frameHeight < 1) return;
  if (!Number.isInteger(candidate.frameWidth) || !Number.isInteger(candidate.frameHeight)) return;
  const key = [candidate.layout,candidate.frameWidth,candidate.frameHeight,candidate.frameCount,candidate.columns||0,candidate.rows||0].join(':');
  if (list.some(x => x.key === key)) return;
  list.push({ ...candidate, key, score: scoreCandidate(candidate) });
}

export function detectFilmstripGeometry(width, height) {
  width = Number(width);
  height = Number(height);
  if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) {
    throw new Error('Invalid image dimensions');
  }

  const candidates = [];

  if (height % width === 0) {
    pushCandidate(candidates, {
      layout: 'vertical',
      frameWidth: width,
      frameHeight: width,
      frameCount: height / width
    });
  }
  if (width % height === 0) {
    pushCandidate(candidates, {
      layout: 'horizontal',
      frameWidth: height,
      frameHeight: height,
      frameCount: width / height
    });
  }

  for (const count of COMMON_FRAME_COUNTS) {
    if (height % count === 0) {
      pushCandidate(candidates, {
        layout: 'vertical',
        frameWidth: width,
        frameHeight: height / count,
        frameCount: count
      });
    }
    if (width % count === 0) {
      pushCandidate(candidates, {
        layout: 'horizontal',
        frameWidth: width / count,
        frameHeight: height,
        frameCount: count
      });
    }
  }

  const maxCell = Math.min(width, height, 1024);
  for (let cell = 8; cell <= maxCell; cell++) {
    if (width % cell || height % cell) continue;
    const columns = width / cell;
    const rows = height / cell;
    const count = columns * rows;
    if (columns < 2 || rows < 2 || count > 256) continue;
    pushCandidate(candidates, {
      layout: 'grid',
      frameWidth: cell,
      frameHeight: cell,
      frameCount: count,
      columns,
      rows
    });
  }

  candidates.sort((a,b) => b.score - a.score);
  const best = candidates[0] || null;
  const second = candidates[1] || null;
  let confidence = 'low';
  if (best) {
    const margin = second ? best.score - second.score : 3;
    if (best.score >= 2.8 && margin >= 1.0) confidence = 'high';
    else if (best.score >= 1.5 && margin >= 0.3) confidence = 'medium';
  }

  return {
    width,
    height,
    best: best ? { ...best, confidence } : null,
    candidates: candidates.slice(0, 8).map(({key,...x}) => x)
  };
}

export const FILMSTRIP_COMMON_FRAME_COUNTS = Object.freeze([...COMMON_FRAME_COUNTS]);
