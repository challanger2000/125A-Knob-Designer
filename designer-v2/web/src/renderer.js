import * as THREE from 'three';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { toRendererDesign } from '../../core/project-model.js';

function hexToCss(hex) {
  return (hex || '#808080').slice(0, 7);
}

function makeMaterial(m) {
  const type = m.type;
  let metalness = 0;
  let roughness = 0.7;
  if (type === 'metallic') { metalness = 0.96; roughness = 0.22; }
  if (type === 'brushed') { metalness = 0.9; roughness = 0.34; }
  if (type === 'matte') { metalness = 0.03; roughness = 0.9; }
  if (type === 'solid') { metalness = 0.0; roughness = 0.55; }

  const shininessInfluence = Math.max(0, Math.min(1, (m.shininess ?? 32) / 128));
  const reflectivity = Math.max(0, Math.min(1, (m.reflectivity ?? 30) / 100));
  roughness = Math.max(0.045, Math.min(0.98, roughness * (1.18 - shininessInfluence * 0.5)));

  const material = new THREE.MeshPhysicalMaterial({
    color: hexToCss(m.color),
    metalness,
    roughness,
    clearcoat: type === 'metallic' ? 0.75 * reflectivity : type === 'solid' ? 0.12 : 0.03,
    clearcoatRoughness: type === 'metallic' ? Math.max(0.04, roughness * 0.45) : roughness,
    reflectivity: Math.max(0.04, reflectivity)
  });

  if (type === 'brushed') {
    material.anisotropy = Math.max(0.15, Math.min(1, (m.brushIntensity ?? 30) / 55));
    material.anisotropyRotation = m.brushDirection === 'linear' ? Math.PI / 2 : 0;
  }
  return material;
}

function makeRoundedGeometry(topRadius, bottomRadius, height, bevelRadius) {
  const half = height / 2;
  const bevel = Math.max(0.01, Math.min(height * 0.22, Math.min(topRadius, bottomRadius) * 0.22, bevelRadius));
  const points = [
    new THREE.Vector2(0, -half),
    new THREE.Vector2(Math.max(0.01, bottomRadius - bevel), -half),
    new THREE.Vector2(bottomRadius, -half + bevel),
    new THREE.Vector2(topRadius, half - bevel),
    new THREE.Vector2(Math.max(0.01, topRadius - bevel), half),
    new THREE.Vector2(0, half)
  ];
  return new THREE.LatheGeometry(points, 128);
}

function addSideDetails(group, layer, topRadius, bottomRadius, height, material) {
  const detail = layer.geometry.sideDetail || 'smooth';
  if (detail === 'smooth') return;

  if (detail === 'grooved') {
    const radius = Math.max(topRadius, bottomRadius) * 1.005;
    const tube = Math.max(0.018, radius * 0.018);
    for (const offset of [-0.24, 0, 0.24]) {
      const torus = new THREE.Mesh(
        new THREE.TorusGeometry(radius, tube, 10, 96),
        material
      );
      torus.rotation.x = Math.PI / 2;
      torus.position.y = offset * height;
      torus.castShadow = true;
      group.add(torus);
    }
    return;
  }

  if (detail === 'knurled') {
    const ribs = 32;
    const radius = Math.max(topRadius, bottomRadius) * 1.015;
    const ribWidth = Math.max(0.025, radius * 0.032);
    const ribDepth = Math.max(0.025, radius * 0.025);
    const ribHeight = height * 0.68;
    for (let i = 0; i < ribs; i++) {
      const a = (i / ribs) * Math.PI * 2;
      const rib = new THREE.Mesh(
        new THREE.BoxGeometry(ribWidth, ribHeight, ribDepth),
        material
      );
      rib.position.set(Math.sin(a) * radius, 0, Math.cos(a) * radius);
      rib.rotation.y = a;
      rib.castShadow = true;
      group.add(rib);
    }
  }
}

function makeLayer(layer) {
  const d = layer.geometry.diameter / 100;
  const h = layer.geometry.height / 100;

  let topRadius = 1.75 * d;
  let bottomRadius = topRadius;
  if (layer.geometry.skirtStyle === 'tapered') bottomRadius *= 1.12;
  if (layer.geometry.skirtStyle === 'angled') bottomRadius *= 0.9;

  const height = Math.max(0.18, h * 1.45);
  const bevel = Math.max(0, (layer.geometry.bevelRadius || 0) / 100 * 1.2);
  const material = makeMaterial(layer.material);
  const group = new THREE.Group();
  const mesh = new THREE.Mesh(makeRoundedGeometry(topRadius, bottomRadius, height, bevel), material);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  group.add(mesh);
  addSideDetails(group, layer, topRadius, bottomRadius, height, material);
  return group;
}

function makeAccentRing(accentRing, topRadius) {
  if (!accentRing?.enabled) return null;
  const material = new THREE.MeshPhysicalMaterial({
    color: hexToCss(accentRing.color || '#657A8F'),
    metalness: 0.92,
    roughness: 0.2,
    clearcoat: 0.8,
    clearcoatRoughness: 0.12
  });
  const ring = new THREE.Mesh(
    new THREE.TorusGeometry(Math.max(0.1, topRadius * 0.86), Math.max(0.018, topRadius * 0.025), 16, 128),
    material
  );
  ring.rotation.x = Math.PI / 2;
  ring.castShadow = true;
  return ring;
}

function makeIndicator(indicator, topRadius) {
  if (!indicator?.enabled) return null;
  const color = hexToCss(indicator.material?.color || '#f0f0f0');
  const mat = new THREE.MeshStandardMaterial({
    color,
    metalness: indicator.material?.metallic ? 0.75 : 0.05,
    roughness: indicator.material?.metallic ? 0.3 : 0.55
  });

  let mesh;
  if (indicator.type === 'dot') {
    const r = Math.max(0.05, indicator.size.radius / 45);
    mesh = new THREE.Mesh(new THREE.CylinderGeometry(r, r, 0.06, 32), mat);
  } else if (indicator.type === 'notch' || indicator.type === 'groove') {
    const w = Math.max(0.04, indicator.size.width / 55);
    const len = Math.max(0.2, indicator.size.length / 20);
    mesh = new THREE.Mesh(new THREE.BoxGeometry(w, 0.06, len), mat);
  } else {
    const w = Math.max(0.04, indicator.size.width / 55);
    const len = Math.max(0.25, indicator.size.length / 18);
    mesh = new THREE.Mesh(new THREE.BoxGeometry(w, 0.07, len), mat);
  }

  const radial = Math.max(0.15, Math.min(0.92, indicator.radialPosition / 100)) * topRadius;
  mesh.position.set(0, 0, radial);
  mesh.castShadow = true;
  return mesh;
}

export class KnobPreviewRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true, preserveDrawingBuffer: true });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x11161c);

    this.pmremGenerator = new THREE.PMREMGenerator(this.renderer);
    const roomEnvironment = new RoomEnvironment();
    this.environmentTexture = this.pmremGenerator.fromScene(roomEnvironment, 0.04).texture;
    this.scene.environment = this.environmentTexture;
    roomEnvironment.dispose?.();

    this.camera = new THREE.PerspectiveCamera(26, 1, 0.1, 100);
    this.camera.position.set(0, 7.4, 7.4);
    this.camera.lookAt(0, 0.25, 0);

    this.group = new THREE.Group();
    this.scene.add(this.group);

    const floor = new THREE.Mesh(
      new THREE.CircleGeometry(3.4, 96),
      new THREE.MeshStandardMaterial({ color: 0x171d24, roughness: 0.96, metalness: 0.03 })
    );
    floor.rotation.x = -Math.PI / 2;
    floor.position.y = -1.28;
    floor.receiveShadow = true;
    this.floor = floor;
    this.scene.add(floor);

    this.hemi = new THREE.HemisphereLight(0xbfd3e6, 0x1b1e23, 1.1);
    this.scene.add(this.hemi);

    this.key = new THREE.DirectionalLight(0xffffff, 3.1);
    this.key.castShadow = true;
    this.key.shadow.mapSize.set(1024, 1024);
    this.scene.add(this.key);

    this.rim = new THREE.DirectionalLight(0x8aa6c4, 1.0);
    this.rim.position.set(-4, 3, -4);
    this.scene.add(this.rim);

    this.resizeObserver = new ResizeObserver(() => this.resize());
    this.resizeObserver.observe(canvas);
    this.resize();
    this.render();
  }

  disposeObject(obj) {
    obj.traverse(child => {
      if (child.geometry) child.geometry.dispose?.();
      if (child.material) {
        const mats = Array.isArray(child.material) ? child.material : [child.material];
        mats.forEach(m => m.dispose?.());
      }
    });
  }

  clearGroup() {
    for (const child of [...this.group.children]) {
      this.group.remove(child);
      this.disposeObject(child);
    }
  }

  update(project) {
    const d = toRendererDesign(project);
    this.clearGroup();

    let y = -1.05;
    let topRadius = 1.75;
    d.layers.forEach(layer => {
      const mesh = makeLayer(layer);
      const hh = Math.max(0.18, (layer.geometry.height / 100) * 1.45);
      mesh.position.y = y + hh / 2;
      y += hh * 0.72;
      topRadius = 1.75 * (layer.geometry.diameter / 100);
      this.group.add(mesh);
    });

    const accentRing = makeAccentRing(d.accentRing, topRadius);
    if (accentRing) {
      accentRing.position.y = y + 0.025;
      this.group.add(accentRing);
    }

    const indicator = makeIndicator(d.indicator, topRadius);
    this.indicatorMesh = indicator || null;
    if (indicator) {
      indicator.position.y = y + 0.05;
      this.group.add(indicator);
    }

    const az = THREE.MathUtils.degToRad(d.lighting.azimuth);
    const el = THREE.MathUtils.degToRad(d.lighting.elevation);
    const radius = 8;
    this.key.position.set(
      Math.cos(el) * Math.cos(az) * radius,
      Math.sin(el) * radius,
      Math.cos(el) * Math.sin(az) * radius
    );

    const ao = Math.max(0, Math.min(100, d.lighting.aoStrength)) / 100;
    this.hemi.intensity = 1.25 - ao * 0.65;
    this.key.intensity = 2.4 + ao * 1.9;

    const previewAngle = (project.output.startAngle + project.output.endAngle) * 0.5;
    this.group.rotation.y = THREE.MathUtils.degToRad(-previewAngle);
    this.render();
  }

  setPreviewAngle(angle) {
    this.group.rotation.y = THREE.MathUtils.degToRad(-angle);
    this.render();
  }

  resize() {
    const rect = this.canvas.getBoundingClientRect();
    const w = Math.max(320, Math.floor(rect.width));
    const h = Math.max(320, Math.floor(rect.height));
    this.renderer.setSize(w, h, false);
    this.camera.aspect = w / h;
    this.camera.updateProjectionMatrix();
    this.render();
  }

  render() {
    this.renderer.render(this.scene, this.camera);
  }

  _filmstripPlan(project) {
    const o = project.output;
    const layout = o.layout || 'vertical';
    if (layout === 'vertical') {
      return { width: o.frameWidth, height: o.frameHeight * o.frameCount, cols: 1, rows: o.frameCount };
    }
    if (layout === 'horizontal') {
      return { width: o.frameWidth * o.frameCount, height: o.frameHeight, cols: o.frameCount, rows: 1 };
    }
    const cols = Math.ceil(Math.sqrt(o.frameCount));
    const rows = Math.ceil(o.frameCount / cols);
    return { width: o.frameWidth * cols, height: o.frameHeight * rows, cols, rows };
  }

  _framePosition(index, project, plan) {
    if ((project.output.layout || 'vertical') === 'vertical') {
      return { x: 0, y: index * project.output.frameHeight };
    }
    if (project.output.layout === 'horizontal') {
      return { x: index * project.output.frameWidth, y: 0 };
    }
    return {
      x: (index % plan.cols) * project.output.frameWidth,
      y: Math.floor(index / plan.cols) * project.output.frameHeight
    };
  }

  _alphaBounds(ctx, width, height) {
    const data = ctx.getImageData(0, 0, width, height).data;
    let minX = width, minY = height, maxX = -1, maxY = -1;
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        if (data[(y * width + x) * 4 + 3] > 8) {
          if (x < minX) minX = x;
          if (x > maxX) maxX = x;
          if (y < minY) minY = y;
          if (y > maxY) maxY = y;
        }
      }
    }
    if (maxX < minX || maxY < minY) return null;
    return {
      minX, minY, maxX, maxY,
      centerX: (minX + maxX) / 2,
      centerY: (minY + maxY) / 2
    };
  }

  async exportFilmstrip(project, onProgress = () => {}) {
    const { frameWidth, frameHeight, frameCount, startAngle, endAngle } = project.output;
    const supersample = Math.max(1, Math.min(4, Number(project.output.supersample || 2)));
    const plan = this._filmstripPlan(project);

    if (plan.width > 32767 || plan.height > 32767) {
      throw new Error(`Filmstrip zu groß (${plan.width} × ${plan.height}). Bitte kleinere Größe, weniger Frames oder Sprite-Grid wählen.`);
    }

    const oldPixelRatio = this.renderer.getPixelRatio();
    const oldBackground = this.scene.background;
    const oldFloorVisible = this.floor.visible;
    const oldRotation = this.group.rotation.y;
    const rect = this.canvas.getBoundingClientRect();
    const restoreWidth = Math.max(320, Math.floor(rect.width));
    const restoreHeight = Math.max(320, Math.floor(rect.height));

    const strip = document.createElement('canvas');
    strip.width = plan.width;
    strip.height = plan.height;
    const ctx = strip.getContext('2d', { alpha: true });
    if (!ctx) throw new Error('PNG-Canvas konnte nicht erstellt werden.');
    ctx.imageSmoothingEnabled = true;
    ctx.imageSmoothingQuality = 'high';

    try {
      this.renderer.setPixelRatio(1);
      this.renderer.setSize(frameWidth * supersample, frameHeight * supersample, false);
      this.camera.aspect = frameWidth / frameHeight;
      this.camera.updateProjectionMatrix();
      this.scene.background = null;
      this.floor.visible = false;

      for (let i = 0; i < frameCount; i++) {
        const t = frameCount <= 1 ? 0 : i / (frameCount - 1);
        const angle = startAngle + (endAngle - startAngle) * t;
        this.group.rotation.y = THREE.MathUtils.degToRad(-angle);
        this.render();

        const p = this._framePosition(i, project, plan);
        ctx.clearRect(p.x, p.y, frameWidth, frameHeight);
        ctx.drawImage(
          this.canvas,
          0, 0, frameWidth * supersample, frameHeight * supersample,
          p.x, p.y, frameWidth, frameHeight
        );

        if (i === 0 || i === frameCount - 1 || i % 8 === 0) {
          onProgress(i + 1, frameCount);
          await new Promise(resolve => requestAnimationFrame(resolve));
        }
      }

      return await new Promise((resolve, reject) => {
        strip.toBlob(blob => blob ? resolve(blob) : reject(new Error('PNG konnte nicht erzeugt werden.')), 'image/png');
      });
    } finally {
      this.group.rotation.y = oldRotation;
      this.scene.background = oldBackground;
      this.floor.visible = oldFloorVisible;
      this.renderer.setPixelRatio(oldPixelRatio);
      this.renderer.setSize(restoreWidth, restoreHeight, false);
      this.camera.aspect = restoreWidth / restoreHeight;
      this.camera.updateProjectionMatrix();
      this.render();
    }
  }

  async measureCenterWobble(project, sampleCount = 9) {
    const { frameWidth, frameHeight, startAngle, endAngle } = project.output;
    const supersample = 2;
    const oldPixelRatio = this.renderer.getPixelRatio();
    const oldBackground = this.scene.background;
    const oldFloorVisible = this.floor.visible;
    const oldRotation = this.group.rotation.y;
    const oldIndicatorVisible = this.indicatorMesh ? this.indicatorMesh.visible : null;
    const rect = this.canvas.getBoundingClientRect();
    const restoreWidth = Math.max(320, Math.floor(rect.width));
    const restoreHeight = Math.max(320, Math.floor(rect.height));

    const sample = document.createElement('canvas');
    sample.width = frameWidth;
    sample.height = frameHeight;
    const ctx = sample.getContext('2d', { alpha: true, willReadFrequently: true });
    if (!ctx) throw new Error('QA-Canvas konnte nicht erstellt werden.');
    ctx.imageSmoothingEnabled = true;
    ctx.imageSmoothingQuality = 'high';

    const centers = [];
    try {
      this.renderer.setPixelRatio(1);
      this.renderer.setSize(frameWidth * supersample, frameHeight * supersample, false);
      this.camera.aspect = frameWidth / frameHeight;
      this.camera.updateProjectionMatrix();
      this.scene.background = null;
      this.floor.visible = false;
      if (this.indicatorMesh) this.indicatorMesh.visible = false;

      for (let i = 0; i < sampleCount; i++) {
        const t = sampleCount <= 1 ? 0 : i / (sampleCount - 1);
        const angle = startAngle + (endAngle - startAngle) * t;
        this.group.rotation.y = THREE.MathUtils.degToRad(-angle);
        this.render();
        ctx.clearRect(0, 0, frameWidth, frameHeight);
        ctx.drawImage(
          this.canvas,
          0, 0, frameWidth * supersample, frameHeight * supersample,
          0, 0, frameWidth, frameHeight
        );
        const bounds = this._alphaBounds(ctx, frameWidth, frameHeight);
        if (bounds) centers.push(bounds);
        await new Promise(resolve => requestAnimationFrame(resolve));
      }

      if (centers.length < 2) throw new Error('Center-QA konnte den Knob nicht zuverlässig erkennen.');
      const xs = centers.map(v => v.centerX);
      const ys = centers.map(v => v.centerY);
      const driftX = Math.max(...xs) - Math.min(...xs);
      const driftY = Math.max(...ys) - Math.min(...ys);
      const maxDrift = Math.hypot(driftX, driftY);
      return {
        driftX,
        driftY,
        maxDrift,
        samples: centers.length,
        pass: maxDrift <= 0.75
      };
    } finally {
      if (this.indicatorMesh && oldIndicatorVisible !== null) this.indicatorMesh.visible = oldIndicatorVisible;
      this.group.rotation.y = oldRotation;
      this.scene.background = oldBackground;
      this.floor.visible = oldFloorVisible;
      this.renderer.setPixelRatio(oldPixelRatio);
      this.renderer.setSize(restoreWidth, restoreHeight, false);
      this.camera.aspect = restoreWidth / restoreHeight;
      this.camera.updateProjectionMatrix();
      this.render();
    }
  }

  dispose() {
    this.resizeObserver.disconnect();
    this.clearGroup();
    this.environmentTexture?.dispose?.();
    this.pmremGenerator?.dispose?.();
    this.renderer.dispose();
  }
}
