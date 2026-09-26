const PRESETS = {
  '125a-studio-dark': {
    name: '125A Studio Dark',
    shape: 'studio',
    material: 'metal-dark',
    baseColor: '#242A31FF',
    accentColor: '#657A8FFF',
    capEnabled: true,
    sideDetail: 'smooth',
    accentRing: true,
    lighting: 'neutral',
    azimuth: 315,
    elevation: 48,
    intensity: 100,
    shadow: 36,
    gloss: 82,
    indicator: 'line',
    indicatorColor: '#E8EEF4FF',
    indicatorLength: 66
  },
  'industrial-black': {
    name: 'Industrial Black',
    shape: 'stepped',
    material: 'soft-touch',
    baseColor: '#171A1EFF',
    accentColor: '#6C7F93FF',
    capEnabled: true,
    sideDetail: 'knurled',
    accentRing: true,
    lighting: 'contrast',
    azimuth: 300,
    elevation: 34,
    intensity: 112,
    shadow: 58,
    gloss: 24,
    indicator: 'line',
    indicatorColor: '#F2F5F7FF',
    indicatorLength: 72
  },
  'brushed-aluminium': {
    name: 'Brushed Aluminium',
    shape: 'compact',
    material: 'aluminium',
    baseColor: '#B8BEC4FF',
    accentColor: '#586C80FF',
    capEnabled: true,
    sideDetail: 'grooved',
    accentRing: true,
    lighting: 'soft',
    azimuth: 322,
    elevation: 58,
    intensity: 108,
    shadow: 25,
    gloss: 74,
    indicator: 'notch',
    indicatorColor: '#20262CFF',
    indicatorLength: 58
  },
  'vintage-brass': {
    name: 'Vintage Brass',
    shape: 'domed',
    material: 'brass',
    baseColor: '#8A6C35FF',
    accentColor: '#34291BFF',
    capEnabled: true,
    sideDetail: 'grooved',
    accentRing: false,
    lighting: 'dramatic',
    azimuth: 292,
    elevation: 30,
    intensity: 92,
    shadow: 66,
    gloss: 68,
    indicator: 'dot',
    indicatorColor: '#F2E6C7FF',
    indicatorLength: 55
  },
  'chrome-modern': {
    name: 'Chrome Modern',
    shape: 'cylindrical',
    material: 'chrome',
    baseColor: '#D9DEE3FF',
    accentColor: '#5C85A8FF',
    capEnabled: false,
    sideDetail: 'smooth',
    accentRing: true,
    lighting: 'contrast',
    azimuth: 305,
    elevation: 44,
    intensity: 125,
    shadow: 42,
    gloss: 120,
    indicator: 'groove',
    indicatorColor: '#1C252DFF',
    indicatorLength: 70
  },
  'rubber-performance': {
    name: 'Rubber Performance',
    shape: 'tapered',
    material: 'rubber',
    baseColor: '#242628FF',
    accentColor: '#C4CED7FF',
    capEnabled: true,
    sideDetail: 'knurled',
    accentRing: false,
    lighting: 'neutral',
    azimuth: 318,
    elevation: 50,
    intensity: 102,
    shadow: 40,
    gloss: 8,
    indicator: 'line',
    indicatorColor: '#F5F7F8FF',
    indicatorLength: 76
  }
};

export function listDesignPresets() {
  return Object.entries(PRESETS).map(([id,preset]) => ({ id, name: preset.name }));
}

export function applyDesignPreset(project, presetId) {
  const preset = PRESETS[presetId];
  if (!preset) throw new Error(`Unknown 125A design preset: ${presetId}`);

  project.design = project.design || {};
  project.design.expert = project.design.expert || {};
  project.design.expert.material = project.design.expert.material || {};
  project.design.indicator = project.design.indicator || {};
  project.lighting = project.lighting || {};
  project.lighting.expert = project.lighting.expert || {};

  project.design.shape = preset.shape;
  project.design.material = preset.material;
  project.design.baseColor = preset.baseColor;
  project.design.accentColor = preset.accentColor;
  project.design.expert.capEnabled = preset.capEnabled;
  project.design.expert.sideDetail = preset.sideDetail;
  project.design.expert.accentRing = preset.accentRing;
  project.design.expert.material.shininess = preset.gloss;

  project.design.indicator.enabled = true;
  project.design.indicator.type = preset.indicator;
  project.design.indicator.color = preset.indicatorColor;
  project.design.indicator.length = preset.indicatorLength;
  if (!Number.isFinite(project.design.indicator.width)) project.design.indicator.width = 3;

  project.lighting.preset = preset.lighting;
  project.lighting.expert.azimuth = preset.azimuth;
  project.lighting.expert.elevation = preset.elevation;
  project.lighting.expert.intensity = preset.intensity;
  project.lighting.expert.aoStrength = preset.shadow;

  return project;
}
