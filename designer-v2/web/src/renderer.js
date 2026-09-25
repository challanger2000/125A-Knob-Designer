import * as THREE from 'three';
import { toRendererDesign } from '../../core/project-model.js';

function hexToCss(hex) {
  return (hex || '#808080').slice(0, 7);
}

function makeMaterial(m) {
  const type = m.type;
  let metalness = 0;
  let roughness = 0.7;
  if (type === 'metallic') { metalness = 0.9; roughness = 0.28; }
  if (type === 'brushed') { metalness = 0.85; roughness = 0.42; }
  if (type === 'matte') { metalness = 0.05; roughness = 0.88; }
  if (type === 'solid') { metalness = 0.0; roughness = 0.55; }

  const shininessInfluence = Math.max(0, Math.min(1, (m.shininess ?? 32) / 128));
  roughness = Math.max(0.08, Math.min(0.98, roughness * (1.15 - shininessInfluence * 0.45)));

  return new THREE.MeshStandardMaterial({
    color: hexToCss(m.color),
    metalness,
    roughness
  });
}

function makeLayer(layer) {
  const d = layer.geometry.diameter / 100;
  const h = layer.geometry.height / 100;

  let topRadius = 1.75 * d;
  let bottomRadius = topRadius;
  if (layer.geometry.skirtStyle === 'tapered') bottomRadius *= 1.12;
  if (layer.geometry.skirtStyle === 'angled') bottomRadius *= 0.9;

  const g = new THREE.CylinderGeometry(topRadius, bottomRadius, Math.max(0.18, h * 1.45), 96, 1, false);
  const mesh = new THREE.Mesh(g, makeMaterial(layer.material));
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  return mesh;
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
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x11161c);

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

    const indicator = makeIndicator(d.indicator, topRadius);
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

  dispose() {
    this.resizeObserver.disconnect();
    this.clearGroup();
    this.renderer.dispose();
  }
}
