#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <random>
#include <fstream>
#include <iterator>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0.0f, 0.0f, 0.0f);

bool isCulling = true;
bool is_reverse = true;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint shaderProgramID;
GLuint cube_VAO, cube_VBO, cube_EBO, cube_VBO_n, cube_EBO_n;
GLuint tet_VAO, tet_VBO, tet_EBO, tet_VBO_n, tet_EBO_n;

GLuint triangleVertexArrayObject_reversed;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 cameraPos = { -2.0f,2.0f,2.0f };

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool is_cube = true;
bool light_on = true;
bool light_rotate = false;
float light_rotate_angle = 0.0f;
float light_rotate_speed = 1.0f;  // ���: �ð� ����, ����: �ݽð� ����
float light_orbit_radius = 2.0f;  // ���� ������

const float LIGHT_DISTANCE_CHANGE = 0.1f;  // �� ���� ����� �Ÿ�
const float MIN_LIGHT_DISTANCE = 1.0f;     // �ּ� �Ÿ�
const float MAX_LIGHT_DISTANCE = 5.0f;     // �ִ� �Ÿ�

unsigned int texture[6];

float vertexData[] = {
	-0.5f, -0.5f, 0.5f, 0.0, 0.0, 1.0, 0.0, 0.0,
0.5f, -0.5f, 0.5f, 0.0, 0.0, 1.0, 1.0, 0.0,
0.5f, 0.5f, 0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,
0.5f, 0.5f, 0.5f, 0.0, 0.0, 1.0, 1.0, 1.0,
-0.5f, 0.5f, 0.5f, 0.0, 0.0, 1.0, 0.0, 1.0,
-0.5f, -0.5f, 0.5f, 0.0, 0.0, 1.0, 0.0, 0.0
};

char* File_To_Buf(const char* file)
{
	ifstream in(file, ios_base::binary);

	if (!in) {
		cerr << file << "���� X";
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
		cerr << path << "���� �� ã��";
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
	//���̴� �ڵ� ���� �ҷ�����
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	//���̴� ��ü �����
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//���̴� ��ü�� ���̴� �ڵ� ���̱�
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//���̴� ��ü �������ϱ�
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	//���̴� ���� ��������
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader ������ ����\n" << errorLog << endl;
		return false;
	}

	//���̴� ��ü �����
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//���̴� ��ü�� ���̴� �ڵ� ���̱�
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//���̴� ��ü �������ϱ�
	glCompileShader(fragmentShader);
	//���̴� ���� ��������
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader ������ ����\n" << errorLog << endl;
		return false;
	}

	//���̴� ���α׷� ����
	shaderProgramID = glCreateProgram();
	//���̴� ���α׷��� ���̴� ��ü���� ���̱�
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	//���̴� ���α׷� ��ũ
	glLinkProgram(shaderProgramID);

	//���̴� ��ü �����ϱ�
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//���α׷� ���� ��������
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program ���� ����\n" << errorLog << endl;
		return false;
	}

	glUniform3f(glGetUniformLocation(shaderProgramID, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	//���̴� ���α׷� Ȱ��ȭ
	glUseProgram(shaderProgramID);

	return true;
}

void Set_Texture() {
	const char* name[6] = {
		"Tex1.png",
		"Tex2.png",
		"Tex3.png",
		"Tex4.png",
		"Tex5.png",
		"Tex6.png"
	};
	BITMAP* bmp;
	int Image_w, Image_h, numberOfChannel;
	for (int i = 0; i < 6; i++)
	{
		glGenTextures(1, &texture[i]);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		unsigned char* data = stbi_load(name[i], &Image_w, &Image_h, &numberOfChannel, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, Image_w, Image_h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
}

bool Set_VAO() {
	Load_Object("cube.obj");

	// ť�� VAO ����
	glGenVertexArrays(1, &cube_VAO);
	glBindVertexArray(cube_VAO);

	// ��ġ ����
	glGenBuffers(1, &cube_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(positionAttribute);

	GLint normalAttribute = glGetAttribLocation(shaderProgramID, "normalAttribute");
	glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(normalAttribute);
	
	GLint TextureAttribute = glGetAttribLocation(shaderProgramID, "vTexCoord");
	glVertexAttribPointer(TextureAttribute, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(TextureAttribute);

	//// ���ü ����
	//Load_Object("tetrahedron.obj");

	//glGenVertexArrays(1, &tet_VAO);
	//glBindVertexArray(tet_VAO);

	//glGenBuffers(1, &tet_VBO);
	//glBindBuffer(GL_ARRAY_BUFFER, tet_VBO);
	//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	//glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glEnableVertexAttribArray(positionAttribute);

	return true;
}

void Viewport1() {
	glUseProgram(shaderProgramID);

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
	projection = glm::translate(projection, glm::vec3(0.0, 0.0, -5.0));

	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	//TR �ʱ�ȭ, transform��ġ ��������
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightcolor");
	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");

	glm::mat4 TR = glm::mat4(1.0f);
	//���� �׸��� ��Ʈ
	if (is_cube)
	{
		glBindVertexArray(cube_VAO);
		for (int i = 0; i < 6; i++)
		{
			glBindTexture(GL_TEXTURE_2D, texture[i]);
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
			glUniform3f(colorLocation, 1, 1, 1);
			glDrawArrays(GL_TRIANGLES, i * 6, (i * 6) + 6);
		}

	}
	else
	{
		glBindVertexArray(tet_VAO);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(colorLocation, 1, 1, 1);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);
	}

	// ���� ��ġ ��� (��ü ȸ���� �����ϰ�)
	glm::vec3 rotated_light_pos = glm::vec3(
		0.0f,
		30.0f,
		0.0f
	);

	// ���� ť�� �׸���
	glBindVertexArray(cube_VAO);
	glm::mat4 light_TR = glm::mat4(1.0f);
	light_TR = glm::translate(light_TR, rotated_light_pos);
	light_TR = glm::scale(light_TR, glm::vec3(0.2f));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(light_TR));
	glUniform3f(colorLocation, 1.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	// ���� ��ġ ������Ʈ (�� �������� ��ȯ)
	glm::vec4 viewLightPos = view * glm::vec4(rotated_light_pos, 1.0f);
	glUniform3f(lightPosLocation, viewLightPos.x, viewLightPos.y, viewLightPos.z);
	glUniform3f(lightColorLocation, 5.0, 5.0, 5.0);
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	Viewport1();

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, WIN_W, WIN_H);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();

	glutTimerFunc(10, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'n':
		is_cube = !is_cube;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	//������ ����
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	//GLEW �ʱ�ȭ�ϱ�
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	if (!Make_Shader_Program()) {
		cerr << "Error: Shader Program ���� ����" << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_VAO()) {
		cerr << "Error: VAO ���� ����" << endl;
		std::exit(EXIT_FAILURE);
	}

	Set_Texture();

	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}