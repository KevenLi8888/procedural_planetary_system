#include "planet/planetarysystem.h"
#include "glm/gtx/transform.hpp"

#include <random>

struct SolarSystemPlanet {
    std::string texture_fname;
    float diameter;             // km
    float rotational_velocity;  // km/h
    float orbital_radius;       // 10^6km
    float orbital_period;       // days
    float orbital_inclination;  // degrees
};

SolarSystemPlanet Sun {
    "resources/images/sun.jpeg",
    1392684,
    0,
    0,
    0,
    0,
};

SolarSystemPlanet Moon {
    "resources/images/moon.jpeg",
    3475,
    16.7,
    0.384,
    27.3,
    5.1,
};

// https://nssdc.gsfc.nasa.gov/planetary/factsheet/
// https://sos.noaa.gov/catalog/datasets/planet-rotations/
// https://www.solarsystemscope.com/textures/
std::vector<SolarSystemPlanet> Planets {
    // Mercury
    SolarSystemPlanet {
        "resources/images/mercury.jpeg",
        4879,
        10.83,
        57.9,
        88,
        7.0,
    },
    // Venus
    SolarSystemPlanet {
        "resources/images/venus.jpeg",
        12104,
        6.52,
        108.2,
        224.7,
        3.4,
    },
    // Earth
    SolarSystemPlanet {
        "resources/images/earth.jpeg",
        12756,
        1574,
        149.6,
        365.2,
        0,
    },
    // Mars
    SolarSystemPlanet {
        "resources/images/mars.jpeg",
        6792,
        866,
        228,
        687,
        1.8,
    },
    // Jupiter
    SolarSystemPlanet {
        "resources/images/jupiter.jpeg",
        142984,
        45583,
        778.5,
        4331,
        1.3,
    },
    // Saturn
    SolarSystemPlanet {
        "resources/images/saturn.jpeg",
        120536,
        36840,
        1432,
        10747,
        2.5,
    },
    // Uranus
    SolarSystemPlanet {
        "resources/images/uranus.jpeg",
        51118,
        14798,
        2867,
        30589,
        0.8,
    },
    // Neptune
    SolarSystemPlanet {
        "resources/images/neptune.jpeg",
        49528,
        9719,
        4515,
        59800,
        1.8,
    },
};

void deletePlanet(Planet *curr) {
    for (auto &p : curr->getChildren()) {
        deletePlanet(p);
    }
    delete curr;
}

PlanetarySystem::~PlanetarySystem() {
    if (m_root != nullptr) deletePlanet(m_root);
}

float scaleDiameter(float diameter) {
    return (log10(diameter) - 3) * 0.5;
}

float scaleOrbitalRadius(float radius) {
    return (log10(radius) - 1.2) * 7;
}

float scaleVelocity(float v) {
    return sqrt(v) * 10;
}

std::vector<RenderShapeData*> PlanetarySystem::generateSolarSystem() {
    std::vector<RenderShapeData*> data;

    SceneMaterial mat;
    mat.cDiffuse = glm::vec4(1, 1, 1, 0);
    mat.blend = 1;
    mat.textureMap.isUsed = true;
    mat.textureMap.repeatU = 1;
    mat.textureMap.repeatV = 1;

    // Add sun
    mat.textureMap.filename = Sun.texture_fname;
    RenderShapeData *sun_shape = new RenderShapeData {{PrimitiveType::PRIMITIVE_SPHERE, mat}, glm::mat4(1)};
    Planet *sun = new Planet(scaleDiameter(Sun.diameter), 0, 0, 0, 0, glm::vec3(0, 1, 0), sun_shape);
    std::vector<Planet*> sun_children;
    data.push_back(sun_shape);

    // Change parameters for other planets
    mat.blend = 0.5;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(0.f, glm::pi<float>() * 2.f);

    // Add planets
    for (auto &planet: Planets) {
        mat.textureMap.filename = planet.texture_fname;
        RenderShapeData *p_shape = new RenderShapeData {{PrimitiveType::PRIMITIVE_SPHERE, mat}, glm::mat4(1)};
        Planet *p = new Planet(scaleDiameter(planet.diameter),
                               scaleVelocity(1 / planet.orbital_period),
                               planet.rotational_velocity / planet.diameter,
                               dist(mt),
                               scaleOrbitalRadius(planet.orbital_radius),
                               computeAxis(planet.orbital_inclination),
                               p_shape);
        sun_children.push_back(p);
        data.push_back(p_shape);
        p->setParent(sun);
    }

    // Add moon to earth
    mat.textureMap.filename = Moon.texture_fname;
    RenderShapeData *moon_shape = new RenderShapeData {{PrimitiveType::PRIMITIVE_SPHERE, mat}, glm::mat4(1)};
    Planet *moon = new Planet(Moon.diameter / 20000.f,
                           scaleVelocity(1 / Moon.orbital_period),
                           Moon.rotational_velocity / Moon.diameter,
                           dist(mt),
                           Moon.orbital_radius * 1.5,
                           computeAxis(Moon.orbital_inclination),
                           moon_shape);
    data.push_back(moon_shape);

    // Set up hierarchy
    moon->setParent(sun_children[2]);
    sun_children[2]->setChildren({ moon });
    sun->setChildren(sun_children);

    m_root = sun;

    return data;
}

void PlanetarySystem::update(float deltaTime) {
    m_root->updateCTM(deltaTime);
}

void getOrbitCtmsHelper(std::vector<glm::mat4> &ctms, Planet *p) {
    auto parent = p->getParent();

    if (parent != nullptr) {
        ctms.push_back(parent->getTranslateMat() * glm::scale(glm::vec3(2 * p->getOrbitRadius())) * p->getOrientMat() * glm::mat4(1));
    }

    for (auto &child: p->getChildren()) {
        getOrbitCtmsHelper(ctms, child);
    }
}

std::vector<glm::mat4> PlanetarySystem::getOrbitCtms() {
    std::vector<glm::mat4> orbit_ctms;

    getOrbitCtmsHelper(orbit_ctms, m_root);

    return orbit_ctms;
}

// Get rotational axis based on inclination degree
glm::vec3 PlanetarySystem::computeAxis(float inclination) {
    glm::vec3 DEFAULT_AXIS = glm::vec3(0, 1, 0);
    if (inclination == 0) return DEFAULT_AXIS;

    auto rotation_mat = glm::rotate(glm::mat4(1), glm::radians(inclination), glm::vec3(1, 0, 0));
    return rotation_mat * glm::vec4(DEFAULT_AXIS, 0);
}
