#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

#include <glm.hpp>
#include <ext.hpp>
#include <gtc/matrix_transform.hpp>

#include <iostream>
#include <random>
#include <fstream>
#include <iterator>
#include <random>

using namespace std;

random_device seeder;
const auto seed = seeder.entropy() ? seeder() : time(nullptr);
mt19937 eng(static_cast<mt19937::result_type>(seed));
uniform_int_distribution<int> rand_cube(0, 5);
uniform_int_distribution<int> rand_tet(0, 3);
uniform_real_distribution<float> rand_color(0.0, 1.0);  // 0.0 ~ 1.0 사이의 랜덤 실수
uniform_real_distribution<float> rand_size(0.01f, 0.05f);  // 크기 랜덤 범위

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0.0f, 0.0f, 0.0f);

bool isCulling = true;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint shaderProgramID;
GLuint Cubeobj;
GLuint CubeVBO;
GLuint CubeEBO;
GLuint Tetobj;
GLuint TetVBO;
GLuint TetEBO;
GLuint Sphereobj;
GLuint SphereVBO;
GLuint SphereEBO;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 cameraPos = { 0.0f,10.0f,20.0f };

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool light_rotate = false;
float light_rotate_angle = 180.0f;
float light_rotate_speed = 1.0f;
float light_orbit_radius = 5.0f;
float light_height = 3.0f;

glm::vec3 light_color = glm::vec3(1.0f);

struct Snowflake {
	glm::vec3 position;
	float speed;
	bool active;
	float size;  // 추가된 크기 변수
};

struct planet {
	glm::vec3 position;
	glm::vec3 color;
	float size;
};

std::vector<planet> planets;
float rotation = 0.0f;
float radius = 5.0f;

const int SNOW_COUNT = 200;  // 눈송이 개수
std::vector<Snowflake> snowflakes;
const float SNOW_AREA = 20.0f;  // 눈이 내리는 영역의 크기
const float SNOW_HEIGHT = 15.0f; // 눈이 시작되는 높이
uniform_real_distribution<float> rand_pos(-SNOW_AREA / 2, SNOW_AREA / 2);
uniform_real_distribution<float> rand_speed(0.02f, 0.08f);

int sierpinski_level = 0;  // 현재 시어핀스키 삼각형의 단계

bool camera_rotate = false;  // 카메라 회전 상태
float camera_angle = 0.0f;   // 카메라 회전 각도
const float CAMERA_ROTATE_SPEED = 1.0f;  // 카메라 회전 속도

float light_intensity = 2.0f;  // 조명 세기

int light_position_index = 0;  // 조명 위치 인덱스
glm::vec3 light_positions[] = {
	glm::vec3(5.0f, light_height, 0.0f),   // 오른쪽
	glm::vec3(-5.0f, light_height, 0.0f),  // 왼쪽
	glm::vec3(0.0f, light_height, 5.0f),   // 앞
	glm::vec3(0.0f, light_height, -5.0f)   // 뒤
};

// 전역 변수로 추가
glm::vec3 current_light_pos;  // 현재 조명 위치를 저장할 변수
glm::vec3 direction_to_object;  // 조명에서 객체로의 방향 벡터

struct Pillar {
	glm::vec3 position;
	float height;
	float alpha;
};

std::vector<Pillar> pillars;
const int NUM_PILLARS = 10;  // 기둥 개수
uniform_real_distribution<float> rand_height(2.0f, 5.0f);  // 기둥 높이 범위
uniform_real_distribution<float> rand_alpha(0.3f, 0.7f);   // 알파값 범위

void set_body(int body_index, glm::mat4* TR);

char* File_To_Buf(const char* file)
{
	ifstream in(file, ios_base::binary);

	if (!in) {
		cerr << file << " 안 열림";
		exit(1);
	}

	in.seekg(0, ios_base::end);
	long len = in.tellg();
	char* buf = new char[len + 1];
	in.seekg(0, ios_base::beg);

	int cnt = -1;
	while (in >> noskipws >> buf[++cnt]) {}
	buf[len] = 0;

	return buf;
}

bool  Load_Object(const char* path) {
	vertexIndices.clear();
	uvIndices.clear();
	normalIndices.clear();
	vertices.clear();
	uvs.clear();
	normals.clear();

	ifstream in(path);
	if (!in) {
		cerr << path << " 안 열림";
		exit(1);
	}

	while (in) {
		string lineHeader;
		in >> lineHeader;
		if (lineHeader == "v") {
			glm::vec3 vertex;
			in >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (lineHeader == "vt") {
			glm::vec2 uv;
			in >> uv.x >> uv.y;
			uvs.push_back(uv);
		}
		else if (lineHeader == "vn") {
			glm::vec3 normal;
			in >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (lineHeader == "f") {
			char a;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			for (int i = 0; i < 3; i++)
			{
				in >> vertexIndex[i] >> a >> uvIndex[i] >> a >> normalIndex[i];
				vertexIndices.push_back(vertexIndex[i] - 1);
				uvIndices.push_back(uvIndex[i] - 1);
				normalIndices.push_back(normalIndex[i] - 1);
			}
		}
	}

	return true;
}

bool Make_Shader_Program() {
	const GLchar* vertexShaderSource = File_To_Buf("vertex_wlight.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment_wlight.glsl");

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader  " << errorLog << endl;
		return false;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader  " << errorLog << endl;
		return false;
	}

	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program  " << errorLog << endl;
		return false;
	}
	glUseProgram(shaderProgramID);

	return true;
}

bool Set_Tet() {
	Load_Object("tet.obj");

	glGenVertexArrays(1, &Tetobj);
	glBindVertexArray(Tetobj);

	glGenBuffers(1, &TetVBO);
	glBindBuffer(GL_ARRAY_BUFFER, TetVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &TetEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TetEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position Ӽ  " << endl;
		return false;
	}
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	std::vector<glm::vec3> calculated_normals(vertices.size(), glm::vec3(0.0f));
	for (size_t i = 0; i < vertexIndices.size(); i += 3) {
		unsigned int idx1 = vertexIndices[i];
		unsigned int idx2 = vertexIndices[i + 1];
		unsigned int idx3 = vertexIndices[i + 2];

		glm::vec3 v1 = vertices[idx2] - vertices[idx1];
		glm::vec3 v2 = vertices[idx3] - vertices[idx1];
		glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

		calculated_normals[idx1] += normal;
		calculated_normals[idx2] += normal;
		calculated_normals[idx3] += normal;
	}

	for (auto& normal : calculated_normals) {
		normal = glm::normalize(normal);
	}

	GLuint normalVBO;
	glGenBuffers(1, &normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glBufferData(GL_ARRAY_BUFFER, calculated_normals.size() * sizeof(glm::vec3), &calculated_normals[0], GL_STATIC_DRAW);

	GLint normalAttribute = glGetAttribLocation(shaderProgramID, "normalAttribute");
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribute);

	glBindVertexArray(0);

	return true;
}

bool Set_Cube() {
	Load_Object("cube.obj");

	glGenVertexArrays(1, &Cubeobj);
	glBindVertexArray(Cubeobj);

	glGenBuffers(1, &CubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &CubeEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position Ӽ  " << endl;
		return false;
	}
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	std::vector<glm::vec3> calculated_normals(vertices.size(), glm::vec3(0.0f));
	for (size_t i = 0; i < vertexIndices.size(); i += 3) {
		unsigned int idx1 = vertexIndices[i];
		unsigned int idx2 = vertexIndices[i + 1];
		unsigned int idx3 = vertexIndices[i + 2];

		glm::vec3 v1 = vertices[idx2] - vertices[idx1];
		glm::vec3 v2 = vertices[idx3] - vertices[idx1];
		glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

		calculated_normals[idx1] += normal;
		calculated_normals[idx2] += normal;
		calculated_normals[idx3] += normal;
	}

	for (auto& normal : calculated_normals) {
		normal = glm::normalize(normal);
	}

	GLuint normalVBO;
	glGenBuffers(1, &normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glBufferData(GL_ARRAY_BUFFER, calculated_normals.size() * sizeof(glm::vec3), &calculated_normals[0], GL_STATIC_DRAW);

	GLint normalAttribute = glGetAttribLocation(shaderProgramID, "normalAttribute");
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribute);

	glBindVertexArray(0);

	return true;
}

bool Set_Sphere() {
	Load_Object("sphere.obj");

	glGenVertexArrays(1, &Sphereobj);
	glBindVertexArray(Sphereobj);

	glGenBuffers(1, &SphereVBO);
	glBindBuffer(GL_ARRAY_BUFFER, SphereVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &SphereEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position 속성 에러" << endl;
		return false;
	}
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	std::vector<glm::vec3> calculated_normals(vertices.size(), glm::vec3(0.0f));
	for (size_t i = 0; i < vertexIndices.size(); i += 3) {
		unsigned int idx1 = vertexIndices[i];
		unsigned int idx2 = vertexIndices[i + 1];
		unsigned int idx3 = vertexIndices[i + 2];

		glm::vec3 v1 = vertices[idx2] - vertices[idx1];
		glm::vec3 v2 = vertices[idx3] - vertices[idx1];
		glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

		calculated_normals[idx1] += normal;
		calculated_normals[idx2] += normal;
		calculated_normals[idx3] += normal;
	}

	for (auto& normal : calculated_normals) {
		normal = glm::normalize(normal);
	}

	GLuint normalVBO;
	glGenBuffers(1, &normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glBufferData(GL_ARRAY_BUFFER, calculated_normals.size() * sizeof(glm::vec3), &calculated_normals[0], GL_STATIC_DRAW);

	GLint normalAttribute = glGetAttribLocation(shaderProgramID, "normalAttribute");
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribute);

	glBindVertexArray(0);

	return true;
}

// 시어핀스키 삼각형을 그리는 함수 수정
void drawSierpinskiTriangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, int level, GLuint modelLocation, GLuint colorLocation) {
	if (level == 0) {
		// 하나의 삼각형 그리기
		std::vector<glm::vec3> triangle = { v1, v2, v3 };
		GLuint tempVBO;
		glGenBuffers(1, &tempVBO);
		glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
		glBufferData(GL_ARRAY_BUFFER, triangle.size() * sizeof(glm::vec3), &triangle[0], GL_STATIC_DRAW);

		GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
		glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDeleteBuffers(1, &tempVBO);
		return;
	}

	// 중점 계산
	glm::vec3 mid1 = (v1 + v2) * 0.5f;
	glm::vec3 mid2 = (v2 + v3) * 0.5f;
	glm::vec3 mid3 = (v3 + v1) * 0.5f;

	// 3개의 작은 삼각형 그리기
	drawSierpinskiTriangle(v1, mid1, mid3, level - 1, modelLocation, colorLocation);
	drawSierpinskiTriangle(mid1, v2, mid2, level - 1, modelLocation, colorLocation);
	drawSierpinskiTriangle(mid3, mid2, v3, level - 1, modelLocation, colorLocation);
}

// 피라미드의 각 면을 그리는 함수
void drawPyramid(glm::mat4 model, int level, GLuint modelLocation, GLuint colorLocation) {
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(colorLocation, 0.5f, 0.5f, 0.5f);

	// 피라미드의 각 면의 정점들
	glm::vec3 top(0.0f, 1.0f, 0.0f);
	glm::vec3 frontLeft(-1.0f, -1.0f, 1.0f);
	glm::vec3 frontRight(1.0f, -1.0f, 1.0f);
	glm::vec3 backLeft(-1.0f, -1.0f, -1.0f);
	glm::vec3 backRight(1.0f, -1.0f, -1.0f);

	// 앞면
	drawSierpinskiTriangle(top, frontLeft, frontRight, level, modelLocation, colorLocation);

	// 왼쪽면
	drawSierpinskiTriangle(top, backLeft, frontLeft, level, modelLocation, colorLocation);

	// 오른쪽면
	drawSierpinskiTriangle(top, frontRight, backRight, level, modelLocation, colorLocation);

	// 뒷면
	drawSierpinskiTriangle(top, backRight, backLeft, level, modelLocation, colorLocation);
}

void Initialize_Pillars() {
	pillars.clear();
	for(int i = 0; i < NUM_PILLARS; i++) {
		Pillar pillar;
		pillar.position = glm::vec3(rand_pos(eng), 0.0f, rand_pos(eng));
		pillar.height = rand_height(eng);
		pillar.alpha = 0.5f;
		pillars.push_back(pillar);
	}
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	isCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(shaderProgramID);

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, glm::vec3(0.0f), cameraUp);
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
	projection = glm::translate(projection, glm::vec3(0.0, 0.0, -10.0));

	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	GLint alphaLocation = glGetUniformLocation(shaderProgramID, "alpha");
	glUniform1f(alphaLocation, 1.0f);

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightcolor");
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	current_light_pos = glm::vec3(
		light_orbit_radius * cos(glm::radians(light_rotate_angle)),
		light_height,
		light_orbit_radius * sin(glm::radians(light_rotate_angle))
	);

	// 조명과 객체 사이의 방향 벡터 계산
	glm::vec3 object_center(0.0f, 1.0f, 0.0f);  // 피라미드의 중심점
	direction_to_object = glm::normalize(object_center - current_light_pos);

	//바닥면
	glBindVertexArray(Cubeobj);
	glm::mat4 TR = glm::mat4(1.0f);
	TR = glm::translate(TR, glm::vec3(0.0f, -0.5f, 0.0f));
	TR = glm::scale(TR, glm::vec3(50.0f, 0.5f, 50.0f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 1.0, 0.5, 0.5);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//조명 큐브
	glm::mat4 light_TR = glm::mat4(1.0f);
	light_TR = glm::translate(light_TR, current_light_pos);
	light_TR = glm::scale(light_TR, glm::vec3(0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(light_TR));
	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//조명
	glm::vec4 viewLightPos = view * glm::vec4(current_light_pos, 1.0f);
	glUniform3f(lightPosLocation, viewLightPos.x, viewLightPos.y, viewLightPos.z);
	glUniform3f(lightColorLocation, light_color.x * light_intensity, light_color.y * light_intensity, light_color.z * light_intensity);

	glBindVertexArray(Tetobj);
	// 시어핀스키 삼각형
	TR = glm::mat4(1.0f);
	TR = glm::scale(TR, glm::vec3(2.5, 2.5, 2.5));
	TR = glm::translate(TR, glm::vec3(0, 1, 0));
	drawPyramid(TR, sierpinski_level, modelLocation, colorLocation);

	// 눈송이 그리기
	glBindVertexArray(Sphereobj);
	for (const auto& snow : snowflakes) {
		if (!snow.active) continue;
		glm::mat4 snow_TR = glm::mat4(1.0f);
		snow_TR = glm::translate(snow_TR, snow.position);
		snow_TR = glm::scale(snow_TR, glm::vec3(snow.size));  // 각 눈송이의 고유 크기 사용
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(snow_TR));
		glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);  // 하얀색
		glDrawElements(GL_TRIANGLES, vertexIndices.size(), GL_UNSIGNED_INT, (void*)0);
	}

	for (const auto& planet : planets) {
		glm::mat4 planet_TR = glm::mat4(1.0f);
		planet_TR = glm::translate(planet_TR, planet.position);
		planet_TR = glm::scale(planet_TR, glm::vec3(planet.size));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(planet_TR));
		glUniform3f(colorLocation, planet.color.x, planet.color.y, planet.color.z);
		glDrawElements(GL_TRIANGLES, vertexIndices.size(), GL_UNSIGNED_INT, (void*)0);
	}

	// 알파 블렌딩 활성화
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 기둥 그리기
	glBindVertexArray(Cubeobj);
	for(const auto& pillar : pillars) {
		glm::mat4 pillar_TR = glm::mat4(1.0f);
		pillar_TR = glm::translate(pillar_TR, pillar.position);
		pillar_TR = glm::scale(pillar_TR, glm::vec3(0.5f, pillar.height, 0.5f));
		pillar_TR = glm::translate(pillar_TR, glm::vec3(0.0f, 0.5f, 0.0f));
		
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(pillar_TR));
		glUniform3f(colorLocation, 0.5f, 0.7f, 1.0f);  // 기둥 색상
		
		// 알파값 설정

		glUniform1f(alphaLocation, pillar.alpha);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();

	if (light_rotate) {
		light_rotate_angle += light_rotate_speed;
		if (light_rotate_angle >= 360.0f) light_rotate_angle -= 360.0f;
		if (light_rotate_angle < 0.0f) light_rotate_angle += 360.0f;
	}

	// 카메라 회전 업데이트
	if (camera_rotate) {
		camera_angle += CAMERA_ROTATE_SPEED;
		if (camera_angle >= 360.0f) camera_angle -= 360.0f;

		// 카메라 위치 업데이트
		float camera_radius = glm::length(glm::vec2(cameraPos.x, cameraPos.z));
		cameraPos.x = camera_radius * sin(glm::radians(camera_angle));
		cameraPos.z = camera_radius * cos(glm::radians(camera_angle));
	}

	// 눈송이 업데이트
	for (auto& snow : snowflakes) {
		if (!snow.active) continue;
		snow.position.y -= snow.speed;

		// 바닥에 닿으면 다시 위로
		if (snow.position.y < 0.0f) {
			snow.position = glm::vec3(rand_pos(eng), SNOW_HEIGHT, rand_pos(eng));
			snow.speed = rand_speed(eng);
		}
	}
	rotation += 1.0f;
	planets[0].position = glm::vec3{
				0,
				cos(glm::radians(rotation)) * radius,
				sin(glm::radians(rotation)) * radius
	};
	planets[1].position = glm::vec3{
				cos(glm::radians(rotation)) * radius,
				cos(glm::radians(rotation)) * radius,
				sin(glm::radians(rotation)) * radius
	};
	planets[2].position = glm::vec3{
				-cos(glm::radians(rotation)) * radius,
				cos(glm::radians(rotation)) * radius,
				sin(glm::radians(rotation)) * radius
	};
	glutTimerFunc(10, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'm':  // 조명 켜기/끄기
		if (light_color == glm::vec3(0.0f))
			light_color = glm::vec3(1.0f);
		else
			light_color = glm::vec3(0.0f);
		break;
	case 'c':  // 조명 색상 랜덤 변경
		light_color = glm::vec3(rand_color(eng), rand_color(eng), rand_color(eng));
		break;
	case 'R':  // 조명 공전 시작 (양방향)
		light_rotate = true;
		light_rotate_speed = 1.0f;
		break;
	case 'r':  // 조명 공전 토글 (음방)
		light_rotate = !light_rotate;  // 토글
		light_rotate_speed = -1.0f;
		break;
	case 's':
		for (auto& snow : snowflakes) {
			snow.active = !snow.active;
			if (snow.active) {
				snow.position = glm::vec3(rand_pos(eng), SNOW_HEIGHT, rand_pos(eng));
				snow.speed = rand_speed(eng);
				snow.size = rand_size(eng);  // 새로운 랜덤 크기 설정
			}
		}
		break;
	case 'q':  // 프로그램 종료
		glutLeaveMainLoop();
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		sierpinski_level = key - '0';
		break;
	case 'y':  // 카메라 y축 회전 켜기/끄기
		camera_rotate = !camera_rotate;
		break;
	case 'n':  // 조명이 객체에 가까워지기
		light_orbit_radius = max(0.1f, light_orbit_radius - 0.1f);
		break;
	case 'f':  // 조명이 객체에서 멀어지기
		light_orbit_radius = min(10.0f, light_orbit_radius + 0.1f);
		break;
	case '+':  // 조명 세기 높이기
		light_intensity = min(5.0f, light_intensity + 0.1f);
		break;
	case '-':  // 조명 세기 낮추기
		light_intensity = max(0.0f, light_intensity - 0.1f);
		break;
	case 'p':  // 새로운 기둥 생성
		Initialize_Pillars();
		break;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	if (!Make_Shader_Program()) {
		cerr << "Error: Shader Program  " << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_Cube()) {
		cerr << "Error: VAO  " << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_Tet()) {
		cerr << "Error: VAO  " << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_Sphere()) {
		cerr << "Error: Sphere VAO" << endl;
		std::exit(EXIT_FAILURE);
	}

	// 눈송이 초기화
	snowflakes.resize(SNOW_COUNT);
	for (auto& snow : snowflakes) {
		snow.position = glm::vec3(rand_pos(eng), SNOW_HEIGHT, rand_pos(eng));
		snow.speed = rand_speed(eng);
		snow.size = rand_size(eng);  // 랜덤 크기 설정
		snow.active = true;
	}

	planets.resize(3);
	planets[0].color = glm::vec3(1, 0, 0);
	planets[0].position = glm::vec3(0, 5, 0);
	planets[0].size = 0.5f;

	planets[1].color = glm::vec3(0, 1, 0);
	planets[1].position = glm::vec3(-5, 5, 0);
	planets[1].size = 0.2f;

	planets[2].color = glm::vec3(0, 0, 1);
	planets[2].position = glm::vec3(5, 5, 0);
	planets[2].size = 0.1f;

	Initialize_Pillars();  // 기둥 초기화 추가

	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}