namespace cube {

Orb CubeApi::orb = {};
Orbit CubeApi::orbit = {};

void CubeApi::setOrbPosition(float x, float y, float z) {
    orb.x = x;
    orb.y = y;
    orb.z = z;
}

void CubeApi::showOrbit() {
    orbit.visible = true;
}

void CubeApi::hideOrbit() {
    orbit.visible = false;
}

void CubeApi::setLfo(int axis, String waveType, float frequency,
                     float phaseOffset) {
    Orbit::Axis* orbitAxis = getAxis(axis);
    if (orbitAxis == nullptr) {
        return;
    }
    Orbit::Lfo::WaveType waveTypeEnum;
    if (waveType == "sin") {
        waveTypeEnum = Orbit::Lfo::Sin;
    } else if (waveType == "triangle") {
        waveTypeEnum = Orbit::Lfo::Triangle;
    } else if (waveType == "saw") {
        waveTypeEnum = Orbit::Lfo::Saw;
    } else if (waveType == "square") {
        waveTypeEnum = Orbit::Lfo::Square;
    } else {
        std::cout << "Error: unknown wave type: " << waveType << std::endl;
        return;
    }

    orbitAxis->type = Orbit::Axis::Lfo;
    orbitAxis->lfo.waveType = waveTypeEnum;
    orbitAxis->lfo.frequency = frequency;
    orbitAxis->lfo.phaseOffset = phaseOffset;
}

void CubeApi::setLfoRange(int axis, float min, float max) {
    Orbit::Axis* orbitAxis = getAxis(axis);
    if (orbitAxis == nullptr) {
        return;
    }
    orbitAxis->lfo.min = min;
    orbitAxis->lfo.max = max;
}

void CubeApi::setEmptyPath(int axis) {
    Orbit::Axis* orbitAxis = getAxis(axis);
    if (orbitAxis == nullptr) {
        return;
    }
    orbitAxis->type = Orbit::Axis::Path;
    orbitAxis->path.keyframes.clear();
}

void CubeApi::addPathKeyframe(int axis, float time, float pos,
                              bool easeIn, bool easeOut) {
    Orbit::Axis* orbitAxis = getAxis(axis);
    if (orbitAxis == nullptr) {
        return;
    }
    orbitAxis->path.keyframes.push_back({
        .time = time,
        .pos = pos,
        .easeIn = easeIn,
        .easeOut = easeOut
    });
}

void CubeApi::setOrbitRotation(float x, float y, float z) {
    orbit.rotation.x = x;
    orbit.rotation.y = y;
    orbit.rotation.z = z;
}

void CubeApi::setOrbitMirror(bool x, bool y, bool z) {
    orbit.mirror.x = x;
    orbit.mirror.y = y;
    orbit.mirror.z = z;
}

void CubeApi::setOrbitIntensity(float intensity) {
    orbit.intensity = intensity;
}

Orbit::Axis* CubeApi::getAxis(int axis) {
    if (axis == 0) {
        return &orbit.x;
    } else if (axis == 1) {
        return &orbit.y;
    } else if (axis == 2) {
        return &orbit.z;
    } else {
        std::cout << "Invalid orbit axis: " << axis << std::endl;
        return nullptr;
    }
}

}  // namespace cube
