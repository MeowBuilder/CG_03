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

using namespace std;

random_device seeder;
const auto seed = seeder.entropy() ? seeder() : time(nullptr);
mt19937 eng(static_cast<mt19937::result_type>(seed));
uniform_real_distribution<float> rand_color(0.0, 1.0);

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0, 0, 0);

GLUquadricObj* Sun;

GLUquadricObj* planet1;
GLUquadricObj* moon1;
glm::vec3 planet1_translate = { 1.0f,0.0f,0.0f };
glm::vec3 planet1_rotate;
glm::vec3 moon1_rotate;

GLUquadricObj* planet2;
GLUquadricObj* moon2;
glm::vec3 planet2_translate = { 1.5f,0.0f,0.0f };
glm::vec3 planet2_rotate;
glm::vec3 moon2_rotate;

GLUquadricObj* planet3;
GLUquadricObj* moon3;
glm::vec3 planet3_translate = { -1.5f,0.0f,0.0f };
glm::vec3 planet3_rotate = { 0.0f,0.0f,0.0f };
glm::vec3 moon3_rotate;

float radius = 2.0f;

bool isCulling = true;
bool yRotate = false;
bool zRotate = false;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint shaderProgramID;

float world_y_rot = 0.0f;
float world_z_rot = 0.0f;
float y_rot = 1.0f;
float z_rot = 1.0f;

bool isSolid = true;
bool isortho = false;

glm::vec3 translate_all = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec4 moonvertex = { 0,0,0,1.0f };

std::vector<glm::vec3> colors = { {1.0,0.0,0.0}, {0.5,0.5,0.0}, {0.0,1.0,0.0}, {0.0,0.5,0.5}, {0.0,0.0,1.0}, {0.5,0.0,0.5} };

bool light_on = true;
bool light_rotate = false;
float light_rotate_angle = 0.0f;
float light_rotate_speed = 1.0f;  // 양수: 시계 방향, 음수: 반시계 방향
float light_orbit_radius = 3.0f;  // 공전 반지름

const float LIGHT_DISTANCE_CHANGE = 0.1f;  // 한 번에 변경될 거리
const float MIN_LIGHT_DISTANCE = 1.0f;     // 최소 거리
const float MAX_LIGHT_DISTANCE = 5.0f;     // 최대 거리

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

GLuint cube_VAO, cube_VBO, cube_EBO, cube_VBO_n, cube_EBO_n;

glm::vec3 light_color = glm::vec3(1.0);
char* File_To_Buf(const char* file)
{
	ifstream in(file, ios_base::binary);

	if (!in) {
		cerr << file << "파일 못찾음";
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
		cerr << path << "파일 못 찾음";
		exit(1);
	}

	//vector<char> lineHeader(istream_iterator<char>{in}, {});

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

bool Set_VAO() {
	Load_Object("cube.obj");

	// 큐브 VAO 설정
	glGenVertexArrays(1, &cube_VAO);
	glBindVertexArray(cube_VAO);

	// 위치 버퍼
	glGenBuffers(1, &cube_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	// 노말 계산 및 설정
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

	// 노말 정규화
	for (auto& normal : calculated_normals) {
		normal = glm::normalize(normal);
	}

	glGenBuffers(1, &cube_VBO_n);
	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO_n);
	glBufferData(GL_ARRAY_BUFFER, calculated_normals.size() * sizeof(glm::vec3), &calculated_normals[0], GL_STATIC_DRAW);

	GLint normalAttribute = glGetAttribLocation(shaderProgramID, "normalAttribute");
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normalAttribute);

	// 인덱스 버퍼
	glGenBuffers(1, &cube_EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	return true;
}

bool Make_Shader_Program() {
	//세이더 코드 파일 불러오기
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	//세이더 객체 만들기
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//세이더 객체에 세이더 코드 붙이기
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//세이더 객체 컴파일하기
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	//세이더 상태 가져오기
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	//세이더 객체 만들기
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//세이더 객체에 세이더 코드 붙이기
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//세이더 객체 컴파일하기
	glCompileShader(fragmentShader);
	//세이더 상태 가져오기
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	//세이더 프로그램 생성
	shaderProgramID = glCreateProgram();
	//세이더 프로그램에 세이더 객체들을 붙이기
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	//세이더 프로그램 링크
	glLinkProgram(shaderProgramID);

	//세이더 객체 삭제하기
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//프로그램 상태 가져오기
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		return false;
	}
	//세이더 프로그램 활성화
	glUseProgram(shaderProgramID);

	return true;
}

void world_rotation(glm::mat4* TR) {
	*TR = glm::rotate(*TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	*TR = glm::rotate(*TR, glm::radians(30.0f), glm::vec3(0.0, 1.0, 0.0));
	*TR = glm::translate(*TR, translate_all);
	*TR = glm::rotate(*TR, glm::radians(world_y_rot), glm::vec3(0.0, 1.0, 0.0));
	*TR = glm::rotate(*TR, glm::radians(world_z_rot), glm::vec3(0.0, 0.0, 1.0));
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	isCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	isSolid ? gluQuadricDrawStyle(Sun, GLU_FILL) : gluQuadricDrawStyle(Sun, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet1, GLU_FILL) : gluQuadricDrawStyle(planet1, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon1, GLU_FILL) : gluQuadricDrawStyle(moon1, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet2, GLU_FILL) : gluQuadricDrawStyle(planet2, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon2, GLU_FILL) : gluQuadricDrawStyle(moon2, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet3, GLU_FILL) : gluQuadricDrawStyle(planet3, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon3, GLU_FILL) : gluQuadricDrawStyle(moon3, GLU_LINE);

	glUseProgram(shaderProgramID);
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightcolor");
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");


	glm::mat4 view = glm::mat4(1.0f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraDir = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(cameraPos, cameraDir, cameraUp);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
	projection = glm::translate(projection, glm::vec3(0.0, 0.0, -5.0));

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	glm::mat4 TR = glm::mat4(1.0f);

	//태양
	TR = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 1.0, 1.0, 0.0);
	gluSphere(Sun, 0.5, 50, 50);

	//행성1
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet1_translate);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[0].x, colors[0].y, colors[0].z);
	gluSphere(planet1, 0.25, 25, 25);

	//위성1
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet1_translate);
	TR = glm::rotate(TR, glm::radians(moon1_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[1].x, colors[1].y, colors[1].z);
	gluSphere(moon1, 0.1, 10, 10);


	//행성2
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet2_translate);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[2].x, colors[2].y, colors[2].z);
	gluSphere(planet2, 0.25, 25, 25);

	//위성2
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet2_translate);
	TR = glm::rotate(TR, glm::radians(moon2_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[3].x, colors[3].y, colors[3].z);
	gluSphere(moon2, 0.1, 10, 10);

	//행성3
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet3_translate);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[4].x, colors[4].y, colors[4].z);
	gluSphere(planet3, 0.25, 25, 25);

	//위성3
	TR = glm::mat4(1.0f);
	TR = glm::translate(TR, planet3_translate);
	TR = glm::rotate(TR, glm::radians(moon3_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, colors[5].x, colors[5].y, colors[5].z);
	gluSphere(moon3, 0.1, 10, 10);

	// 조명 위치 계산 (물체 회전과 무관하게)
	glm::vec3 rotated_light_pos = glm::vec3(
		light_orbit_radius * cos(glm::radians(light_rotate_angle)),
		0.0f,
		light_orbit_radius * sin(glm::radians(light_rotate_angle))
	);

	// 조명 큐브 그리기
	glBindVertexArray(cube_VAO);
	glm::mat4 light_TR = glm::mat4(1.0f);
	light_TR = glm::translate(light_TR, rotated_light_pos);
	light_TR = glm::scale(light_TR, glm::vec3(0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(light_TR));
	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	// 조명 위치 업데이트 (뷰 공간으로 변환)
	glm::vec4 viewLightPos = view * glm::vec4(rotated_light_pos, 1.0f);
	glUniform3f(lightPosLocation, viewLightPos.x, viewLightPos.y, viewLightPos.z);
	glUniform3f(lightColorLocation, light_color.x, light_color.y, light_color.z);

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();
	if (yRotate) world_y_rot += y_rot;
	if (zRotate) world_z_rot += z_rot;

	if (light_rotate) {
		light_rotate_angle += light_rotate_speed;
		if (light_rotate_angle >= 360.0f) light_rotate_angle -= 360.0f;
		if (light_rotate_angle < 0.0f) light_rotate_angle += 360.0f;
	}
	glutTimerFunc(10, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'c':
		light_color = glm::vec3(rand_color(eng), rand_color(eng), rand_color(eng));
		break;
	case 'r':
		if (!light_rotate) {
			light_rotate = true;
			light_rotate_speed = 1.0f;
		}
		else if (light_rotate_speed > 0) {
			light_rotate_speed = -1.0f;
		}
		else {
			light_rotate = false;
		}
		break;
	case 'h':
		isCulling = 1 - isCulling;
		break;
	case 'y':
		yRotate = !yRotate;
		y_rot = 0.5f;
		break;
	case 'Y':
		yRotate = !yRotate;
		y_rot = -0.5f;
		break;
	case 'z':
		zRotate = !zRotate;
		z_rot = 0.5f;
		break;
	case 'Z':
		zRotate = !zRotate;
		z_rot = -0.5f;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	case 'p': //직각 / 원근투영
		isortho = !isortho;
		break;
	case 'm':
		isSolid = !isSolid;
		break;
	case 'w':
		translate_all.y += 0.1f;
		break;
	case 'a':
		translate_all.x -= 0.1f;
		break;
	case 's':
		translate_all.y -= 0.1f;
		break;
	case 'd':
		translate_all.x += 0.1f;
		break;
	case '+':
		translate_all.z += 0.1f;
		break;
	case '-':
		translate_all.z -= 0.1f;
		break;
	};
	glutPostRedisplay();
}

GLvoid specialKeyboard(int key, int x, int y) {
	switch (key)
	{
	default:
		break;
	}
}

int main(int argc, char** argv)
{
	//윈도우 생성
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	//GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	if (!Make_Shader_Program()) {
		cerr << "Error: Shader Program 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}

	Set_VAO();

	glutTimerFunc(10, TimerFunction1, 1);

	Sun = gluNewQuadric();
	gluQuadricNormals(Sun, GLU_SMOOTH);
	gluQuadricOrientation(Sun, GLU_OUTSIDE);


	planet1 = gluNewQuadric();
	gluQuadricNormals(planet1, GLU_SMOOTH);
	gluQuadricOrientation(planet1, GLU_OUTSIDE);


	moon1 = gluNewQuadric();
	gluQuadricNormals(moon1, GLU_SMOOTH);
	gluQuadricOrientation(moon1, GLU_OUTSIDE);


	planet2 = gluNewQuadric();
	gluQuadricNormals(planet2, GLU_SMOOTH);
	gluQuadricOrientation(planet2, GLU_OUTSIDE);

	moon2 = gluNewQuadric();
	gluQuadricNormals(moon2, GLU_SMOOTH);
	gluQuadricOrientation(moon2, GLU_OUTSIDE);

	planet3 = gluNewQuadric();
	gluQuadricNormals(planet3, GLU_SMOOTH);
	gluQuadricOrientation(planet3, GLU_OUTSIDE);


	moon3 = gluNewQuadric();
	gluQuadricNormals(moon3, GLU_SMOOTH);
	gluQuadricOrientation(moon3, GLU_OUTSIDE);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(specialKeyboard);
	glutMainLoop();
}