namespace cube {

CubeApi::Orb CubeApi::orb = {};

int CubeApi::setOrbPosition(float x, float y, float z) {
    orb.x = x;
    orb.y = y;
    orb.z = z;
    return 0;
}

}
