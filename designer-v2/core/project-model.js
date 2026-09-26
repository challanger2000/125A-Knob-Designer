const SHAPE_PRESETS = {
  studio: {
    layers: [
      { name: 'Body', diameter: 100, height: 82, bevelRadius: 6, skirtStyle: 'cylindrical' },
      { name: 'Cap', diameter: 82, height: 18, bevelRadius: 4, skirtStyle: 'cylindrical' }
    ]
  },
  flat: {
    layers: [
      { name: 'Body', diameter: 100, height: 55, bevelRadius: 2, skirtStyle: 'cylindrical' }
    ]
  },
  tapered: {
    layers: [
      { name: 'Body', diameter: 100, height: 78, bevelRadius: 5, skirtStyle: 'tapered' },
      { name: 'Cap', diameter: 76, height: 22, bevelRadius: 4, skirtStyle: 'cylindrical' }
    ]
  },
  cylindrical: {
    layers: [
      { name: 'Body', diameter: 100, height: 100, bevelRadius: 3, skirtStyle: 'cylindrical' }
    ]
  },
  stepped: {
    layers: [
      { name: 'Skirt', diameter: 100, height: 30, bevelRadius: 3, skirtStyle: 'cylindrical' },
      { name: 'Body', diameter: 84, height: 52, bevelRadius: 5, skirtStyle: 'cylindrical' },
      { name: 'Cap', diameter: 70, height: 18, bevelRadius: 4, skirtStyle: 'cylindrical' }
    ]
  },
  domed: {
    layers: [
      { name: 'Body', diameter: 100, height: 76, bevelRadius: 14, skirtStyle: 'tapered' },
      { name: 'Cap', diameter: 78, height: 24, bevelRadius: 10, skirtStyle: 'cylindrical' }
    ]
  },
  compact: {
    layers: [
      { name: 'Body', diameter: 100, height: 48, bevelRadius: 9, skirtStyle: 'cylindrical' },
      { name: 'Cap', diameter: 74, height: 14, bevelRadius: 7, skirtStyle: 'cylindrical' }
    ]
  }
};

const MATERIAL_PRESETS = {
  'plastic-black': { type: 'matte', color: '#22262BFF', shininess: 8, reflectivity: 4, brushDirection: 'radial', brushIntensity: 0 },
  'metal-dark': { type: 'metallic', color: '#343B43FF', shininess: 82, reflectivity: 54, brushDirection: 'radial', brushIntensity: 0 },
  aluminium: { type: 'brushed', color: '#B7BDC3FF', shininess: 72, reflectivity: 58, brushDirection: 'radial', brushIntensity: 36 },
  steel: { type: 'brushed', color: '#69727AFF', shininess: 62, reflectivity: 50, brushDirection: 'linear', brushIntensity: 28 },
  brass: { type: 'metallic', color: '#9A7B3FFF', shininess: 74, reflectivity: 48, brushDirection: 'radial', brushIntensity: 0 },
  chrome: { type: 'metallic', color: '#D7DBDEFF', shininess: 118, reflectivity: 92, brushDirection: 'radial', brushIntensity: 0 },
  gold: { type: 'metallic', color: '#C49A3AFF', shininess: 102, reflectivity: 78, brushDirection: 'radial', brushIntensity: 0 },
  'soft-touch': { type: 'matte', color: '#303338FF', shininess: 4, reflectivity: 2, brushDirection: 'radial', brushIntensity: 0 },
  rubber: { type: 'matte', color: '#252729FF', shininess: 2, reflectivity: 1, brushDirection: 'radial', brushIntensity: 0 }
};

const LIGHTING_PRESETS = {
  soft: { azimuth: 315, elevation: 62, aoStrength: 22 },
  neutral: { azimuth: 315, elevation: 48, aoStrength: 36 },
  contrast: { azimuth: 300, elevation: 36, aoStrength: 52 },
  dramatic: { azimuth: 292, elevation: 28, aoStrength: 68 }
};

function clamp(n,min,max){ return Math.max(min,Math.min(max,n)); }
function clone(v){ return JSON.parse(JSON.stringify(v)); }

function mergeExpertLighting(base, expert={}) {
  return {
    azimuth: clamp(Number.isFinite(expert.azimuth) ? expert.azimuth : base.azimuth, 0, 360),
    elevation: clamp(Number.isFinite(expert.elevation) ? expert.elevation : base.elevation, 0, 90),
    aoStrength: clamp(Number.isFinite(expert.aoStrength) ? expert.aoStrength : base.aoStrength, 0, 100),
    intensity: clamp(Number.isFinite(expert.intensity) ? expert.intensity : 100, 25, 200)
  };
}

function materialForProject(project) {
  const preset = clone(MATERIAL_PRESETS[project.design.material] ?? MATERIAL_PRESETS['plastic-black']);
  preset.color = project.design.baseColor || preset.color;
  const expert = project.design.expert?.material ?? {};
  if (['solid','metallic','matte','brushed'].includes(expert.type)) preset.type = expert.type;
  if (Number.isFinite(expert.shininess)) preset.shininess = clamp(expert.shininess,0,128);
  if (Number.isFinite(expert.reflectivity)) preset.reflectivity = clamp(expert.reflectivity,0,100);
  if (['radial','linear'].includes(expert.brushDirection)) preset.brushDirection = expert.brushDirection;
  if (Number.isFinite(expert.brushIntensity)) preset.brushIntensity = clamp(expert.brushIntensity,0,100);
  return preset;
}

function layersForProject(project) {
  const shape = clone(SHAPE_PRESETS[project.design.shape] ?? SHAPE_PRESETS.studio);
  const material = materialForProject(project);
  const overrides = Array.isArray(project.design.expert?.layers) ? project.design.expert.layers : [];
  const capEnabled = project.design.expert?.capEnabled !== false;
  const sideDetail = ['smooth','grooved','knurled'].includes(project.design.expert?.sideDetail)
    ? project.design.expert.sideDetail : 'smooth';
  return shape.layers
    .filter(g => capEnabled || g.name !== 'Cap')
    .map((g,index) => {
    const o = overrides[index] ?? {};
    return {
      id: `layer-${index+1}`,
      name: o.name || g.name,
      geometry: {
        diameter: clamp(Number.isFinite(o.diameter) ? o.diameter : g.diameter,10,100),
        height: clamp(Number.isFinite(o.height) ? o.height : g.height,10,100),
        bevelRadius: clamp(Number.isFinite(o.bevelRadius) ? o.bevelRadius : g.bevelRadius,0,20),
        skirtStyle: ['cylindrical','tapered','angled'].includes(o.skirtStyle) ? o.skirtStyle : g.skirtStyle,
        sideDetail: o.sideDetail || (g.name === 'Body' ? sideDetail : 'smooth')
      },
      material: { ...material, ...(o.material ?? {}) }
    };
  });
}

function indicatorForProject(project) {
  const i = project.design.indicator;
  if (!i?.enabled) return null;
  const lenPct = clamp(Number.isFinite(i.length) ? i.length : 66, 0, 100);
  const width = clamp(Number.isFinite(i.width) ? i.width : 3, 0.5, 20);
  return {
    enabled: true,
    type: i.type,
    material: { color: i.color, metallic: Boolean(project.design.expert?.indicatorMetallic) },
    size: {
      radius: Math.max(1.5, width * 1.2),
      length: 4 + lenPct * 0.22,
      width,
      height: Math.max(1.2, width * 0.65),
      depth: Math.max(1, width * 0.6)
    },
    radialPosition: clamp(Number.isFinite(project.design.expert?.indicatorRadialPosition)
      ? project.design.expert.indicatorRadialPosition : 72,10,90)
  };
}

export function toRendererDesign(project) {
  const lightBase = LIGHTING_PRESETS[project.lighting.preset] ?? LIGHTING_PRESETS.neutral;
  const lighting = mergeExpertLighting(lightBase, project.lighting.expert);
  return {
    id: '',
    name: project.name || '125A Knob',
    layers: layersForProject(project),
    indicator: indicatorForProject(project),
    accentRing: {
      enabled: project.design.expert?.accentRing !== false,
      color: project.design.accentColor || '#657A8FFF'
    },
    lighting,
    output: {
      frameCount: project.output.frameCount,
      frameWidth: project.output.frameWidth,
      frameHeight: project.output.frameHeight,
      sweepAngle: project.output.endAngle - project.output.startAngle,
      startAngle: project.output.startAngle,
      endAngle: project.output.endAngle,
      layout: project.output.layout,
      rotationOffset: Number.isFinite(project.design.expert?.rotationOffset)
        ? project.design.expert.rotationOffset : 0
    },
    cameraView: project.design.expert?.cameraView === 'side' ? 'side' : 'top'
  };
}

export function calculateFilmstripDimensions(project) {
  const o = project.output;
  if (o.layout === 'vertical') return { width:o.frameWidth, height:o.frameHeight*o.frameCount };
  if (o.layout === 'horizontal') return { width:o.frameWidth*o.frameCount, height:o.frameHeight };
  const cols = Math.ceil(Math.sqrt(o.frameCount));
  const rows = Math.ceil(o.frameCount/cols);
  return { width:o.frameWidth*cols, height:o.frameHeight*rows, columns:cols, rows };
}

export function validateExportPlan(project, maxCanvasDimension=32767) {
  const dimensions = calculateFilmstripDimensions(project);
  const supersample = Number(project.output.supersample ?? 2);
  const errors = [];
  if (!Number.isFinite(supersample) || supersample < 1 || supersample > 4) {
    errors.push('supersample must be between 1 and 4');
  }
  if (dimensions.width > maxCanvasDimension || dimensions.height > maxCanvasDimension) {
    errors.push(`filmstrip canvas exceeds ${maxCanvasDimension}px: ${dimensions.width}x${dimensions.height}`);
  }
  const renderWidth = project.output.frameWidth * supersample;
  const renderHeight = project.output.frameHeight * supersample;
  if (renderWidth > maxCanvasDimension || renderHeight > maxCanvasDimension) {
    errors.push(`supersampled frame exceeds ${maxCanvasDimension}px: ${renderWidth}x${renderHeight}`);
  }
  return {
    ok: errors.length === 0,
    errors,
    dimensions,
    supersample,
    renderWidth,
    renderHeight
  };
}

export function calculateFrameAngle(project,index) {
  const count = project.output.frameCount;
  const t = count <= 1 ? 0 : clamp(index,0,count-1)/(count-1);
  return project.output.startAngle + (project.output.endAngle-project.output.startAngle)*t;
}

export const SIMPLE_OPTIONS = Object.freeze({
  shapes:Object.keys(SHAPE_PRESETS),
  materials:Object.keys(MATERIAL_PRESETS),
  lighting:Object.keys(LIGHTING_PRESETS),
  indicators:['line','dot','notch','groove']
});
