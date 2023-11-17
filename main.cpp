#include <SDL.h>
#include <ctime>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <string>

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;



class Color {
public:
    uint8_t r, g, b, a;
};

Color clearColor = {0, 0, 0, 255};
Color currentColor = {255, 255, 255, 255};

SDL_Renderer* renderer;

struct Face {
    std::array<int, 3> vertexIndices;
};

struct Uniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewport;
};




glm::mat4 createModelMatrix(glm::vec3 positionInMap, float scale_) {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), positionInMap);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale_, scale_, scale_));
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return translation * scale * rotation;
}



glm::mat4 createViewMatrix(glm::vec3 positionInMap, float rotation) {
    // Calcular la dirección hacia la que mira la cámara
    glm::vec3 front;
    front.x = cos(glm::radians(rotation));
    front.y = 0.0f; // La cámara mira horizontalmente en este caso
    front.z = sin(glm::radians(rotation));
    front = glm::normalize(front);

    // Calcular la posición de la cámara después de la rotación
    glm::vec3 newPosition = positionInMap;
    glm::vec3 orientation = newPosition + front;

    return glm::lookAt(
            newPosition,
            orientation,
            glm::vec3(0, 1, 0)
    );
}

glm::mat4 createProjectionMatrix(float scale) {
    float fovInDegrees = 45.0f;
    float aspectRatio = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;  // Use the window dimensions
    float nearClip = 0.1f * scale;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

void configureUniform(Uniform* u) {
    // Update the function calls with appropriate parameters
    u->model = createModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
    u->view = createViewMatrix(glm::vec3(0.0f, 0.0f, 5.0f), 0.0f);
    u->projection = createProjectionMatrix(1.0f);
    u->viewport = createViewportMatrix();
}


void point(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y);
}

void line(const glm::vec3& start, const glm::vec3& end) {
    SDL_RenderDrawLine(renderer, static_cast<int>(start.x), static_cast<int>(start.y),
                       static_cast<int>(end.x), static_cast<int>(end.y));
}

void triangle(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    line(A, B);
    line(B, C);
    line(C, A);
}


glm::vec3 barycentricCoordinates(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
    // Uso de fórmulas de coordenadas bariocéntricas
    float detT = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
    float alpha = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) / detT;
    float beta = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) / detT;
    float gamma = 1.0f - alpha - beta;

    return glm::vec3(alpha, beta, gamma);
}

glm::vec3 calculateFlatShadingColor(const glm::vec3& normal, const glm::vec3& lightDirection, const glm::vec3& lightColor) {
    float intensity = glm::dot(glm::normalize(normal), glm::normalize(-lightDirection));
    intensity = glm::clamp(intensity, 0.0f, 1.0f); // Clamping to ensure the intensity is in the valid range [0, 1]

    return intensity * lightColor;
}



void fillTriangle(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    SDL_Rect rect;
    rect.w = 1;  // Ancho del píxel
    rect.h = 1;  // Altura del píxel

    // Calcular la normal del triángulo
    glm::vec3 edge1 = B - A;
    glm::vec3 edge2 = C - A;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    // Calcular la dirección de la luz
    glm::vec3 L = glm::vec3(0, 0, 200.0f);
    glm::vec3 lightDirection = glm::normalize(L - A);

    // Calcular la intensidad de la luz en el triángulo
    float intensity = glm::dot(normal, lightDirection) * 0.5f; // Ajusta el factor según sea necesario


    // Iterar sobre los píxeles del triángulo
    for (int y = glm::min(glm::min(A.y, B.y), C.y); y <= glm::max(glm::max(A.y, B.y), C.y); ++y) {
        for (int x = glm::min(glm::min(A.x, B.x), C.x); x <= glm::max(glm::max(A.x, B.x), C.x); ++x) {
            glm::vec3 point = glm::vec3(x, y, 0);

            // Verificar si el punto está dentro del triángulo usando coordenadas bariocéntricas
            glm::vec3 barycentric = barycentricCoordinates(glm::vec2(point.x, point.y),
                                                           glm::vec2(A.x, A.y),
                                                           glm::vec2(B.x, B.y),
                                                           glm::vec2(C.x, C.y));

            float alpha = barycentric.x;
            float beta = barycentric.y;
            float gamma = barycentric.z;

            if (alpha > 0 && beta > 0 && gamma > 0) {
                rect.x = x;
                rect.y = y;

                // Interpolar el color en función de la intensidad de la luz
                Color interpolatedColor;
                interpolatedColor.r = static_cast<uint8_t>(currentColor.r * intensity);
                interpolatedColor.g = static_cast<uint8_t>(currentColor.g * intensity);
                interpolatedColor.b = static_cast<uint8_t>(currentColor.b * intensity);

                SDL_SetRenderDrawColor(renderer, interpolatedColor.r, interpolatedColor.g, interpolatedColor.b, currentColor.a);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}




void render(const std::vector<glm::vec3>& vertex, const Uniform& uniforms) {
    SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);

    for (size_t i = 0; i < vertex.size(); i += 3) {
        const glm::vec3& A = vertex[i];
        const glm::vec3& B = vertex[i + 1];
        const glm::vec3& C = vertex[i + 2];

        // Apply transformation using the provided uniforms
        glm::vec4 A_transformed = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(A, 1.0f);
        glm::vec4 B_transformed = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(B, 1.0f);
        glm::vec4 C_transformed = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(C, 1.0f);

        int offsetX = WINDOW_WIDTH / 2;
        int offsetY = WINDOW_HEIGHT / 2;

        glm::vec3 A_screen = glm::vec3(A_transformed) + glm::vec3(offsetX, offsetY, 0);
        glm::vec3 B_screen = glm::vec3(B_transformed) + glm::vec3(offsetX, offsetY, 0);
        glm::vec3 C_screen = glm::vec3(C_transformed) + glm::vec3(offsetX, offsetY, 0);

        fillTriangle(A_screen, B_screen, C_screen);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(1000 / 60);
}


std::vector<glm::vec3> setupVertex(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces)
{
    std::vector<glm::vec3> vertex;
    float scale = 1.0f;

    for (const auto& face : faces)
    {
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];

        glm::vec3 vertexScaled1 = vertexPosition1 * scale;
        glm::vec3 vertexScaled2 = vertexPosition2 * scale;
        glm::vec3 vertexScaled3 = vertexPosition3 * scale;

        vertex.push_back(vertexScaled1);
        vertex.push_back(vertexScaled2);
        vertex.push_back(vertexScaled3);
    }

    return vertex;
}

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_vertices;
    std::vector<std::array<int, 3>> temp_faces;

    glm::vec3 centroid(0.0f); // Inicializar el centroide en (0, 0, 0)

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);

            centroid += vertex;
        } else if (type == "f") {
            std::array<int, 3> face_indices{};
            for (int i = 0; i < 3; i++) {
                std::string faceIndexStr;
                iss >> faceIndexStr;

                size_t pos = faceIndexStr.find_first_of('/');
                if (pos != std::string::npos) {
                    faceIndexStr = faceIndexStr.substr(0, pos);
                }

                face_indices[i] = std::stoi(faceIndexStr); // No restar 1
            }
            temp_faces.push_back(face_indices);
        }
    }


    centroid /= static_cast<float>(temp_vertices.size());
    float rotationAngleY = 0.0f;
    glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

    float rotationAngleX = 0.0f;
    glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleX), glm::vec3(1.0f, 0.0f, 0.0f));

    float rotationAngleZ = 180.0f;
    glm::mat4 rotationMatrixZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 combinedRotationMatrix = rotationMatrixY * rotationMatrixX * rotationMatrixZ;


    out_vertices.reserve(temp_vertices.size());
    for (const auto& vertex : temp_vertices) {

        glm::vec3 centeredVertex = (vertex - centroid) * 30.0f;

        glm::vec4 rotatedVertex = combinedRotationMatrix * glm::vec4(centeredVertex, 1.0f);

        out_vertices.push_back(glm::vec3(rotatedVertex));
    }

    out_faces.reserve(temp_faces.size() * 2);
    for (const auto& face : temp_faces) {
        if (face.size() == 3) {
            Face f{};
            f.vertexIndices = { face[0] - 1, face[1] - 1, face[2] - 1 };
            out_faces.push_back(f);
        } else if (face.size() == 4) {

            Face f1{}, f2{};
            f1.vertexIndices = { face[0] - 1, face[1] - 1, face[2] - 1 };
            f2.vertexIndices = { face[0] - 1, face[2] - 1, face[3] - 1 };
            out_faces.push_back(f1);
            out_faces.push_back(f2);
        }
    }




    return true;
}





int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    srand(time(nullptr));

    SDL_Window* window = SDL_CreateWindow("Pixel Drawer", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    Uniform uniforms;
    configureUniform(&uniforms);

    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;

    float rotationAngle = 0.0f; // Inicializa el ángulo de rotación

    bool success = loadOBJ("../lab3.obj", vertices, faces);
    if (!success) {
        return 1;
    }

    std::vector<glm::vec3> vertex = setupVertex(vertices, faces);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        rotationAngle += 0.1f;

        // Model transformation (rotation)
        uniforms.model = createModelMatrix(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f) * glm::rotate(glm::mat4(1.0f), rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        uniforms.view = createViewMatrix(glm::vec3(0.0f, 0.0f, 5.0f), 0.0f);
        uniforms.projection = createProjectionMatrix(1.0f);

        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);

        render(vertex, uniforms);
        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}