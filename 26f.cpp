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

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0.0f, 0.0f, 0.0f);

bool isCulling = true;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint triangleVertexArrayObject;
GLuint shaderProgramID;
GLuint trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
GLuint trianglePositionElementBufferObject;
GLuint Line_VAO, Line_VBO;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 world_rotate = { 0.0,0.0,0.0 };
bool world_rotate_y = false;
float world_y_seta = 0.5f;

bool isortho = false;

glm::vec3 trans_down_body = { 0.0f,0.0f,0.0f };
glm::vec3 rotate_mid_body = { 0.0f,0.0f,0.0f };
bool rotate_mid = false;
float rotate_mid_seta = 1.0f;

glm::vec3 rotate_barrel_body = { 0.0f,0.0f,0.0f };
bool rotate_barrel = false;
float rotate_barrel_seta = 1.0f;

float barrel_trans_mid = 0.5f;
bool make_barrel_one = false;

glm::vec3 rotate_arms = { 0.0f,0.0f,0.0f };
bool rotate_arms_bool = false;
float rotate_arm_seta = 0.5f;

//ī޶ zx
glm::vec3 cameraPos = { 0.0f,1.0f,5.0f };
glm::vec3 camera_rotate = { 0.0f,0.0f,0.0f };
bool camera_rotate_y = false;
float camera_y_seta = 0.5f;
bool camera_rotate_all = false;

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool light_rotate = false;
float light_rotate_angle = 0.0f;
float light_rotate_speed = 1.0f;
float light_orbit_radius = 10.0f;  // 공전 반지름
float light_height = 3.0f;        // y축 높이

glm::vec3 light_color = glm::vec3(1.0f);  // 조명 색상

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

bool Make_Shader_Program() {
	//̴ڵ ҷ
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	//̴ü
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//̴ü ̴ڵ ̱
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//̴ü ϱ
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	//̴
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader  " << errorLog << endl;
		return false;
	}

	//̴ü
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//̴ü ̴ڵ ̱
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//̴ü ϱ
	glCompileShader(fragmentShader);
	//̴
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader  " << errorLog << endl;
		return false;
	}

	//̴α׷
	shaderProgramID = glCreateProgram();
	//̴α׷ ̴ü ̱
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	//̴α׷ ũ
	glLinkProgram(shaderProgramID);

	//̴ü ϱ
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//α׷
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program  " << errorLog << endl;
		return false;
	}
	//̴α׷ Ȱȭ
	glUseProgram(shaderProgramID);

	return true;
}

bool Set_VAO() {
	Load_Object("cube.obj");

	glGenVertexArrays(1, &Line_VAO);
	glBindVertexArray(Line_VAO);
	glGenBuffers(1, &Line_VBO);

	//ؽ Ʈ (VAO) ̸
	glGenVertexArrays(1, &triangleVertexArrayObject);
	//VAO εѴ.
	glBindVertexArray(triangleVertexArrayObject);

	//Vertex Buffer Object(VBO) Ͽ vertex ͸ Ѵ.

	//ؽ Ʈ (VBO) ̸
	glGenBuffers(1, &trianglePositionVertexBufferObjectID);
	// Ʈ εѴ.
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	// Ʈ ͸
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//Ʈ Ʈ (EBO) ̸
	glGenBuffers(1, &trianglePositionElementBufferObject);
	// Ʈ εѴ.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglePositionElementBufferObject);
	// Ʈ ͸
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	//ġ Լ
	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position Ӽ  " << endl;
		return false;
	}
	//ؽ Ӽ  Ʈ 
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//ؽ Ӽ  Ʈ ϵѴ.
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

	// 노말 버퍼 생성 및 설정
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

void world_trans(glm::mat4* TR) {

}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	isCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(shaderProgramID);
	glm::vec3 rotatedTarget = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(world_rotate.y), glm::vec3(0.0f, 1.0f, 0.0)) * glm::vec4(cameraTarget - cameraPos, 1.0f)) + cameraPos;

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, rotatedTarget, cameraUp);
	view = glm::rotate(view, glm::radians(camera_rotate.x), glm::vec3(1.0, 0.0, 0.0));
	view = glm::rotate(view, glm::radians(camera_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	view = glm::rotate(view, glm::radians(camera_rotate.z), glm::vec3(0.0, 0.0, 1.0));
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	if (isortho)
	{
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -10.0f, 10.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
		projection = glm::translate(projection, glm::vec3(0.0, 0.0, -10.0));
	}

	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightcolor");
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");

	// 조명 위치 계산 (drawScene 함수 내 도형 그리기 전)
	glm::vec3 rotated_light_pos = glm::vec3(
		light_orbit_radius * cos(glm::radians(light_rotate_angle)),
		light_height,
		light_orbit_radius * sin(glm::radians(light_rotate_angle))
	);

	// 
	glBindVertexArray(triangleVertexArrayObject);

	//ٴ
	glm::mat4 TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(0, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 1, 1, 1);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//Ʒü
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(1, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0, 0, 1);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//ü
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(2, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 1, 0, 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(3, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0, 1, 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(4, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0, 1, 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//ũ
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(5, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0.5, 0.5, 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//ũ
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(6, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glUniform3f(colorLocation, 0.5, 0.5, 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	// 조명 그리기
	glBindVertexArray(triangleVertexArrayObject);
	glm::mat4 light_TR = glm::mat4(1.0f);
	light_TR = glm::translate(light_TR, rotated_light_pos);
	light_TR = glm::scale(light_TR, glm::vec3(0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(light_TR));
	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);  // 흰색 큐브
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	// 조명 위치를 뷰 공간으로 변환하여 셰이더에 전달
	glm::vec4 viewLightPos = view * glm::vec4(rotated_light_pos, 1.0f);
	glUniform3f(lightPosLocation, viewLightPos.x, viewLightPos.y, viewLightPos.z);
	glUniform3f(lightColorLocation, light_color.x * 2.0, light_color.y * 2.0, light_color.z * 2.0);

	glutSwapBuffers();
}

void set_body(int body_index, glm::mat4* TR) {
	*TR = glm::translate(*TR, trans_down_body);
	switch (body_index)
	{
	case 0://ٴ
		*TR = glm::translate(*TR, glm::vec3(0.0f, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(50.0f, 0.5f, 50.0f));
		break;
	case 1://Ʒü
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.0f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(1.5f, 0.5f, 1.5f));
		break;
	case 2://ü
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.375f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.75f, 0.25f, 0.75f));
		break;
	case 3://
		*TR = glm::rotate(*TR, glm::radians(-rotate_barrel_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(-barrel_trans_mid, 0.0f, 1.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 0.1, 0.75f));
		break;
	case 4://
		*TR = glm::rotate(*TR, glm::radians(rotate_barrel_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(barrel_trans_mid, 0.0f, 1.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 0.1, 0.75f));
		break;
	case 5://ũ
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.2f, 0.25f, 0.0f));
		*TR = glm::rotate(*TR, glm::radians(rotate_arms.z), glm::vec3(0.0, 0.0, 1.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.25f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.15, 0.5f, 0.15f));
		break;
	case 6://ũ
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(-0.2f, 0.25f, 0.0f));
		*TR = glm::rotate(*TR, glm::radians(-rotate_arms.z), glm::vec3(0.0, 0.0, 1.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.25f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.15, 0.5f, 0.15f));
		break;
	default:
		break;
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();
	if (camera_rotate_y)
	{
		camera_rotate.y += camera_y_seta;
	}
	if (world_rotate_y)
	{
		world_rotate.y += world_y_seta;
	}
	if (camera_rotate_all)
	{
		camera_rotate.x += camera_y_seta;
		camera_rotate.z += camera_y_seta;
	}

	if (rotate_mid)
	{
		rotate_mid_body.y += rotate_mid_seta;
	}
	if (rotate_barrel)
	{
		rotate_barrel_body.y += rotate_barrel_seta;
	}
	if (rotate_arms_bool)
	{
		if (rotate_arms.z < 90.0f && rotate_arms.z > -90.0f)
		{
			rotate_arms.z += rotate_arm_seta;
			if (rotate_arms.z >= 90.0f || rotate_arms.z <= -90.0f)
				rotate_arms.z -= rotate_arm_seta;
		}
	}

	if (make_barrel_one)
	{
		if (rotate_barrel_body.y < 0)
		{
			rotate_barrel_body.y += 0.5f;
		}
		else if (rotate_barrel_body.y > 0)
		{
			rotate_barrel_body.y -= 0.5f;
		}
		else if (rotate_barrel_body.y == 0)
		{
			if (barrel_trans_mid > 0)
			{
				barrel_trans_mid -= 0.01f;
			}
		}
	}
	else
	{
		if (barrel_trans_mid < 0.5f)
		{
			barrel_trans_mid += 0.01f;
		}
	}

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
	case 'y':  // 조명 공전 시작 (양방향)
		light_rotate = true;
		light_rotate_speed = 1.0f;
		break;
	case 'Y':  // 조명 공전 시작 (음방향)
		light_rotate = true;
		light_rotate_speed = -1.0f;
		break;
	case 's':  // 모든 회전 정지
		light_rotate = false;
		camera_rotate_y = false;
		break;
	case 'z':  // 카메라 z축 이동
		cameraPos.z += 0.1f;
		break;
	case 'Z':
		cameraPos.z -= 0.1f;
		break;
	case 'x':  // 카메라 x축 이동
		cameraPos.x += 0.1f;
		break;
	case 'X':
		cameraPos.x -= 0.1f;
		break;
	case 'r':  // 카메라 y축 공전 (양방향)
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = 0.5f;
		break;
	case 'R':  // 카메라 y축 공전 (음방향)
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = -0.5f;
		break;
	case 'q':  // 프로그램 종료
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	// 
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	//GLEW ʱȭϱ
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

	if (!Set_VAO()) {
		cerr << "Error: VAO  " << endl;
		std::exit(EXIT_FAILURE);
	}
	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}