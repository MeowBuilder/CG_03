﻿#include <GL/glew.h>
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

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 cameraPos = { 0.0f,1.0f,5.0f };

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool light_rotate = false;
float light_rotate_angle = 180.0f;
float light_rotate_speed = 1.0f;
float light_orbit_radius = 5.0f;
float light_height = 3.0f;

glm::vec3 light_color = glm::vec3(1.0f);

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
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

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
	Load_Object("tetrahedron.obj");

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

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightcolor");
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	glm::vec3 rotated_light_pos = glm::vec3(
		light_orbit_radius * cos(glm::radians(light_rotate_angle)),
		light_height,
		light_orbit_radius * sin(glm::radians(light_rotate_angle))
	);

	//바닥면
	glBindVertexArray(Cubeobj);
	glm::mat4 TR = glm::mat4(1.0f);
	set_body(0, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0.5, 0.5, 0.5);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);
	
	//조명 큐브
	glm::mat4 light_TR = glm::mat4(1.0f);
	light_TR = glm::translate(light_TR, rotated_light_pos);
	light_TR = glm::scale(light_TR, glm::vec3(0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(light_TR));
	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//조명
	glm::vec4 viewLightPos = view * glm::vec4(rotated_light_pos, 1.0f);
	glUniform3f(lightPosLocation, viewLightPos.x, viewLightPos.y, viewLightPos.z);
	glUniform3f(lightColorLocation, light_color.x, light_color.y, light_color.z);

	glBindVertexArray(Tetobj);
	//중앙 피라미드
	TR = glm::mat4(1.0f);
	TR = glm::scale(TR, glm::vec3(2.5, 2.5, 2.5));
	TR = glm::translate(TR, glm::vec3(0, 0.5, 0));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0.5, 0.5, 0.5);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);


	glutSwapBuffers();
}

void set_body(int body_index, glm::mat4* TR) {
	*TR = glm::translate(*TR, glm::vec3(0.0f, -0.5f, 0.0f));
	*TR = glm::scale(*TR, glm::vec3(50.0f, 0.5f, 50.0f));
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
	case 'r':  // 조명 공전 시작 (음방향)
		light_rotate = true;
		light_rotate_speed = -1.0f;
		break;
	case 's':  // 모든 회전 정지
		light_rotate = false;
		break;
	case 'q':  // 프로그램 종료
		glutLeaveMainLoop();
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
	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}