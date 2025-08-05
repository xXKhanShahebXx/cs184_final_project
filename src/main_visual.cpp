#include "Cloth.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <GL/glut.h>

Cloth* cloth = nullptr;
float cameraDistance = 10.0f;
float cameraAngleX = 30.0f;
float cameraAngleY = 0.0f;

std::vector<unsigned char> textureData;
int textureWidth = 64;
int textureHeight = 64;

Vec3 windVelocity = Vec3(0.0f, 0.0f, 0.0f);
float windStrength = 2.0f;
float airDragCoefficient = 0.1f;
bool windActive = false;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glTranslatef(0.0f, -2.0f, -cameraDistance);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);
    
    glBegin(GL_LINES);
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(3.0f, 0.0f, 0.0f);
    
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 3.0f, 0.0f);
    
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 3.0f);
    glEnd();
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glRasterPos3f(3.2f, 0.0f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
    
    glColor3f(0.0f, 1.0f, 0.0f);
    glRasterPos3f(0.0f, 3.2f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
    
    glColor3f(0.0f, 0.0f, 1.0f);
    glRasterPos3f(0.0f, 0.0f, 3.2f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');
    
    if (windActive) {
        glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(windVelocity.x * 0.5f, windVelocity.y * 0.5f, windVelocity.z * 0.5f);
        glEnd();
        
        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos3f(windVelocity.x * 0.6f, windVelocity.y * 0.6f, windVelocity.z * 0.6f);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'W');
    }
    
    {
        const auto& particles = cloth->getParticles();
        int gridW = static_cast<int>(std::sqrt(particles.size()));
        int gridH = gridW;
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());
        
        glBegin(GL_QUADS);
        for (int y = 0; y < gridH - 1; ++y) {
            for (int x = 0; x < gridW - 1; ++x) {
                int idx = y * gridW + x;
                const Vec3& p1 = particles[idx].position;
                const Vec3& p2 = particles[idx + 1].position;
                const Vec3& p3 = particles[idx + 1 + gridW].position;
                const Vec3& p4 = particles[idx + gridW].position;
                
                float u1 = static_cast<float>(x) / (gridW - 1);
                float v1 = static_cast<float>(y) / (gridH - 1);
                float u2 = static_cast<float>(x + 1) / (gridW - 1);
                float v2 = static_cast<float>(y + 1) / (gridH - 1);
                
                glTexCoord2f(u1, v1); glVertex3f(p1.x, p1.y, p1.z);
                glTexCoord2f(u2, v1); glVertex3f(p2.x, p2.y, p2.z);
                glTexCoord2f(u2, v2); glVertex3f(p3.x, p3.y, p3.z);
                glTexCoord2f(u1, v2); glVertex3f(p4.x, p4.y, p4.z);
            }
        }
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    glBegin(GL_LINES);
    glColor3f(0.8f, 0.8f, 0.8f);
    
    const auto& springs = cloth->getSprings();
    const auto& particles = cloth->getParticles();
    
    for (const auto& spring : springs) {
        const auto& p1 = particles[spring.particle1];
        const auto& p2 = particles[spring.particle2];
        
        glVertex3f(p1.position.x, p1.position.y, p1.position.z);
        glVertex3f(p2.position.x, p2.position.y, p2.position.z);
    }
    glEnd();
    
    glBegin(GL_POINTS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glPointSize(3.0f);
    
    for (const auto& particle : particles) {
        if (particle.fixed) {
            glColor3f(0.0f, 1.0f, 0.0f);
        } else {
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        glVertex3f(particle.position.x, particle.position.y, particle.position.z);
    }
    glEnd();

    glutSwapBuffers();
}

void generateTexture() {
    textureData.resize(textureWidth * textureHeight * 3);
    
    for (int y = 0; y < textureHeight; ++y) {
        for (int x = 0; x < textureWidth; ++x) {
            int index = (y * textureWidth + x) * 3;
            
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            
            if (checker) {
                textureData[index] = 200;
                textureData[index + 1] = 180;
                textureData[index + 2] = 160;
            } else {
                textureData[index] = 160;
                textureData[index + 1] = 140;
                textureData[index + 2] = 120;
            }
        }
    }
}

void update() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    deltaTime = std::min(deltaTime, 0.016f);
    
    Vec3 gravity(0.0f, -2.0f, 0.0f);
    Vec3 airVel = windActive ? windVelocity : Vec3(0.0f, 0.0f, 0.0f);

    cloth->update(deltaTime, gravity, airDragCoefficient, airVel);

    if (windActive) {
        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            std::cout << "Wind active: " << airVel.x << ", " << airVel.y << ", " << airVel.z << std::endl;
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27:
            exit(0);
            break;
        case 'w':
            cameraAngleX += 5.0f;
            break;
        case 's':
            cameraAngleX -= 5.0f;
            break;
        case 'a':
            cameraAngleY -= 5.0f;
            break;
        case 'd':
            cameraAngleY += 5.0f;
            break;
        case '+':
        case '=':
            cameraDistance -= 0.5f;
            break;
        case '-':
            cameraDistance += 0.5f;
            break;
        case '1':
            windVelocity = Vec3(windStrength, 0.0f, 0.0f);
            windActive = true;
            std::cout << "Wind: X-axis, Strength: " << windStrength << std::endl;
            break;
        case '2':
            windVelocity = Vec3(0.0f, windStrength, 0.0f);
            windActive = true;
            std::cout << "Wind: Y-axis, Strength: " << windStrength << std::endl;
            break;
        case '3':
            windVelocity = Vec3(0.0f, 0.0f, windStrength);
            windActive = true;
            std::cout << "Wind: +Z axis, Strength: " << windStrength << std::endl;
            break;
        case '7':
            windVelocity = Vec3(-windStrength, 0.0f, 0.0f);
            windActive = true;
            std::cout << "Wind: -X axis, Strength: " << windStrength << std::endl;
            break;
        case '8':
            windVelocity = Vec3(0.0f, -windStrength, 0.0f);
            windActive = true;
            std::cout << "Wind: -Y axis, Strength: " << windStrength << std::endl;
            break;
        case '9':
            windVelocity = Vec3(0.0f, 0.0f, -windStrength);
            windActive = true;
            std::cout << "Wind: -Z axis, Strength: " << windStrength << std::endl;
            break;
        case '4':
            windStrength = std::min(10.0f, windStrength + 1.0f);
            if (windActive) {
                windVelocity = windVelocity.normalize() * windStrength;
                std::cout << "Wind strength increased to: " << windStrength << std::endl;
            }
            break;
        case '5':
            windStrength = std::max(0.0f, windStrength - 1.0f);
            if (windActive) {
                windVelocity = windVelocity.normalize() * windStrength;
                std::cout << "Wind strength decreased to: " << windStrength << std::endl;
            }
            break;
        case '0':
            windActive = false;
            std::cout << "Wind turned OFF" << std::endl;
            break;
        case 'r':
            delete cloth;
            cloth = new Cloth(15, 15, 0.15f);
            cloth->fixCorner(0);
            windActive = false;
            windStrength = 2.0f;
            std::cout << "Cloth reset to original position with fixed corner" << std::endl;
            break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    std::cout << "Cloth Simulation - Stage 3 (Wind Effects)" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W/S - Rotate camera up/down" << std::endl;
    std::cout << "  A/D - Rotate camera left/right" << std::endl;
    std::cout << "  +/- - Zoom in/out" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  Wind Controls:" << std::endl;
    std::cout << "    1/2/3 - Wind +X/+Y/+Z" << std::endl;
    std::cout << "    7/8/9 - Wind -X/-Y/-Z" << std::endl;
    std::cout << "    4/5 - Increase/decrease wind strength" << std::endl;
    std::cout << "    0 - Turn wind off" << std::endl;
    std::cout << "    R - Reset cloth" << std::endl;
    std::cout << std::endl;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Cloth Simulation - Stage 3");
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    generateTexture();
    
    cloth = new Cloth(15, 15, 0.15f);
    cloth->fixCorner(0);
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(update);
    
    std::cout << "Created cloth with " << cloth->getParticles().size() << " particles" << std::endl;
    std::cout << "Created " << cloth->getSprings().size() << " springs" << std::endl;
    std::cout << "Window opened - you should see the cloth falling!" << std::endl;
    std::cout << "Initial wind strength: " << windStrength << std::endl;
    std::cout << "Coordinate System:" << std::endl;
    std::cout << "  Red axis (X) = Left/Right" << std::endl;
    std::cout << "  Green axis (Y) = Up/Down" << std::endl;
    std::cout << "  Blue axis (Z) = Forward/Backward" << std::endl;
    std::cout << "  Yellow arrow (W) = Wind direction" << std::endl;
    std::cout << "Press 1/2/3 to activate wind in different directions!" << std::endl;
    
    glutMainLoop();
    
    delete cloth;
    return 0;
} 