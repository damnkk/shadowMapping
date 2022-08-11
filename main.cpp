#include<iostream>>
#include<string>
#include<fstream>
#include<vector>
#include<map>
#include<sstream>

//glew glut
#include  <GL\glew.h>
#include<GL/freeglut.h>

//glm
#include<glm/glm.hpp>
#include<glm\gtc\matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include<SOIL2\SOIL2.h>

//assimp
#include<assimp\Importer.hpp>
#include<assimp\scene.h>
#include<assimp\postprocess.h>


class Mesh//一个网格对象
{
public:

	GLuint vao, vbo, ebo;
	GLuint diffuseTexture;

	std::vector<glm::vec3> vertexPosition;
	std::vector<glm::vec2> vertexTexCoord;
	std::vector<glm::vec3> vertexNormal;

	//glDrawElements函数的绘制索引
	std::vector<int> index;
	Mesh() {}
	void bindData()
	{
		//创建顶点数组对象
		glGenVertexArrays(1, &vao);//分配一个顶点数组对象
		glBindVertexArray(vao);

		//创建并初始化顶点缓存对象,这里填NULL,先不传数据
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertexPosition.size() * sizeof(glm::vec3) + vertexTexCoord.size() * sizeof(glm::vec2) + vertexNormal.size() * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);

		//传位置
		GLuint offset_position = 0;
		GLuint size_position = vertexPosition.size() * sizeof(glm::vec3);
		glBufferSubData(GL_ARRAY_BUFFER, offset_position, size_position, vertexPosition.data());
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_position));//为什么这里的步长坐标都是0,因为我们这里没有混着
 //放这个数据,之前我们是ABC,ABC,ABC。现在是AAABBBCCC就不用考虑步长的问题了

		//传纹理坐标
		GLuint offset_texcoord = size_position;
		GLuint size_texcoord = vertexTexCoord.size() * sizeof(glm::vec2);
		glBufferSubData(GL_ARRAY_BUFFER, offset_texcoord, size_texcoord, vertexTexCoord.data());
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_texcoord));

		//传法向量
		GLuint offset_normal = offset_texcoord + size_texcoord;
		GLuint size_normal = vertexNormal.size() * sizeof(glm::vec3);
		glBufferSubData(GL_ARRAY_BUFFER, offset_normal, size_normal, vertexNormal.data());
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_normal));

		//传索引到ebo
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(GLuint), index.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	void draw(GLuint program)
	{
		glBindVertexArray(vao);

		//传纹理
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseTexture);
		glUniform1i(glGetUniformLocation(program, "texture"), 0);

		//绘制
		glDrawElements(GL_TRIANGLES, this->index.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
};

class Model
{
public:
	std::vector<Mesh> meshes;
	std::map<std::string, GLuint> textureMap;
	glm::vec3 translate = glm::vec3(0, 0, 0), rotate = glm::vec3(0, 0, 0), scale = glm::vec3(1, 1, 1);
	Model() {}
	void load(std::string filepath)
	{
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
		// 异常处理
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "读取模型出现错误: " << import.GetErrorString() << std::endl;
			exit(-1);
		}
		// 模型文件相对路径
		std::string rootPath = filepath.substr(0, filepath.find_last_of('/'));

		// 循环生成 mesh
		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			// 引用当前mesh
			meshes.push_back(Mesh());
			Mesh& mesh = meshes.back();

			// 获取 assimp 的读取到的 aimesh 对象
			aiMesh* aimesh = scene->mMeshes[i];

			// 我们将数据传递给我们自定义的mesh
			for (int j = 0; j < aimesh->mNumVertices; j++)
			{
				// 顶点
				glm::vec3 vvv;
				vvv.x = aimesh->mVertices[j].x;
				vvv.y = aimesh->mVertices[j].y;
				vvv.z = aimesh->mVertices[j].z;
				mesh.vertexPosition.push_back(vvv);

				// 法线
				vvv.x = aimesh->mNormals[j].x;
				vvv.y = aimesh->mNormals[j].y;
				vvv.z = aimesh->mNormals[j].z;
				mesh.vertexNormal.push_back(vvv);

				// 纹理坐标: 如果存在则加入。assimp 默认可以有多个纹理坐标 我们取第一个（0）即可
				glm::vec2 vv(0, 0);
				if (aimesh->mTextureCoords[0])
				{
					vv.x = aimesh->mTextureCoords[0][j].x;
					vv.y = aimesh->mTextureCoords[0][j].y;
				}
				mesh.vertexTexCoord.push_back(vv);
			}

			// 如果有材质，那么传递材质
			if (aimesh->mMaterialIndex >= 0)
			{
				// 获取当前 aimesh 的材质对象
				aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];

				// 获取 diffuse 贴图文件路径名称 我们只取1张贴图 故填 0 即可
				aiString aistr;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);
				std::string texpath = aistr.C_Str();
				texpath = rootPath + '/' + texpath;   // 取相对路径

				// 如果没生成过纹理，那么生成它
				if (textureMap.find(texpath) == textureMap.end())
				{
					// 生成纹理
					GLuint tex;
					glGenTextures(1, &tex);
					glBindTexture(GL_TEXTURE_2D, tex);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
					int textureWidth, textureHeight;
					unsigned char* image = SOIL_load_image(texpath.c_str(), &textureWidth, &textureHeight, 0, SOIL_LOAD_RGB);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);   // 生成纹理
					delete[] image;

					textureMap[texpath] = tex;
				}

				// 传递纹理
				mesh.diffuseTexture = textureMap[texpath];
			}

			// 传递面片索引
			for (GLuint j = 0; j < aimesh->mNumFaces; j++)
			{
				aiFace face = aimesh->mFaces[j];
				for (GLuint k = 0; k < face.mNumIndices; k++)
				{
					mesh.index.push_back(face.mIndices[k]);
				}
			}

			mesh.bindData();
		}
	}
	void draw(GLuint program)
	{
		// 传模型矩阵
		glm::mat4 unit(    // 单位矩阵
			glm::vec4(1, 0, 0, 0),
			glm::vec4(0, 1, 0, 0),
			glm::vec4(0, 0, 1, 0),
			glm::vec4(0, 0, 0, 1)
		);
		glm::mat4 scale = glm::scale(unit, this->scale);
		glm::mat4 translate = glm::translate(unit, this->translate);

		glm::mat4 rotate = unit;    // 旋转
		rotate = glm::rotate(rotate, glm::radians(this->rotate.x), glm::vec3(1, 0, 0));
		rotate = glm::rotate(rotate, glm::radians(this->rotate.y), glm::vec3(0, 1, 0));
		rotate = glm::rotate(rotate, glm::radians(this->rotate.z), glm::vec3(0, 0, 1));

		// 模型变换矩阵
		glm::mat4 model = translate * rotate * scale;
		GLuint mlocation = glGetUniformLocation(program, "model");    // 名为model的uniform变量的位置索引
		glUniformMatrix4fv(mlocation, 1, GL_FALSE, glm::value_ptr(model));   // 列优先矩阵

		for (int i = 0; i < meshes.size(); i++)
		{
			meshes[i].draw(program);
		}
	}
};

class Camera
{
public:
	// 相机参数
	glm::vec3 position = glm::vec3(0, 0, 0);    // 位置
	glm::vec3 direction = glm::vec3(0, 0, -1);  // 视线方向
	glm::vec3 up = glm::vec3(0, 1, 0);          // 上向量，固定(0,1,0)不变
	float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;    // 欧拉角
	float fovy = 70.0f, aspect = 1.0, zNear = 0.01, zFar = 100; // 透视投影参数
	float left = -1.0, right = 1.0, top = 1.0, bottom = -1.0; // 正交投影参数
	Camera() {}
	// 视图变换矩阵
	glm::mat4 getViewMatrix(bool useEulerAngle = true)
	{
		if (useEulerAngle)  // 使用欧拉角更新相机朝向
		{
			direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
			direction.y = sin(glm::radians(pitch));
			direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw)); // 相机看向z轴负方向
		}
		return glm::lookAt(position, position + direction, up);
	}
	// 投影矩阵
	glm::mat4 getProjectionMatrix(bool usePerspective = true)
	{
		if (usePerspective) // 透视投影
		{
			return glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
		}
		return glm::ortho(left, right, bottom, top, zNear, zFar);
	}
};

//--------------------end of class definition--------------------//

//模型
std::vector<Model> models;
Model screen;

//着色器程序对象
GLuint program;
GLuint debugProgram;
GLuint shadowProgram;

//相机
Camera camera;
Camera shadowCamera;

//光源与阴影参数
int shadowMapResolution = 1024;
GLuint shadowMapFBO;
GLuint shadowTexture;

int windowWidth = 512;
int windowHeight = 512;
bool keyboardState[1024];

//-------------------end of global variable definition--------------------//

// 读取文件并且返回一个长字符串表示文件内容
std::string readShaderFile(std::string filepath)
{
	std::string res, line;
	std::ifstream fin(filepath);
	if (!fin.is_open())
	{
		std::cout << "文件 " << filepath << " 打开失败" << std::endl;
		exit(-1);
	}
	while (std::getline(fin, line))
	{
		res += line + '\n';
	}
	fin.close();
	return res;
}

GLuint getShaderProgram(std::string fshader, std::string vshader)
{
	// 读取shader源文件
	std::string vSource = readShaderFile(vshader);
	std::string fSource = readShaderFile(fshader);
	const char* vpointer = vSource.c_str();
	const char* fpointer = fSource.c_str();

	// 容错
	GLint success;
	GLchar infoLog[512];

	// 创建并编译顶点着色器
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar**)(&vpointer), NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);   // 错误检测
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "顶点着色器编译错误\n" << infoLog << std::endl;
		exit(-1);
	}

	// 创建并且编译片段着色器
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar**)(&fpointer), NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);   // 错误检测
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "片段着色器编译错误\n" << infoLog << std::endl;
		exit(-1);
	}

	// 链接两个着色器到program对象
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// 删除着色器对象
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void mouseWheel(int wheel, int direction, int x, int y)
{
	// zFar += 1 * direction * 0.1;
	glutPostRedisplay();    // 重绘
}

// 鼠标运动函数
void mouse(int x, int y)
{
	// 调整旋转
	camera.yaw += 35 * (x - float(windowWidth) / 2.0) / windowWidth;
	camera.yaw = glm::mod(camera.yaw + 180.0f, 360.0f) - 180.0f;    // 取模范围 -180 ~ 180

	camera.pitch += -35 * (y - float(windowHeight) / 2.0) / windowHeight;
	camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

	glutWarpPointer(windowWidth / 2.0, windowHeight / 2.0);
	glutPostRedisplay();    // 重绘
}
//键盘回调函数

void keyboardDown(unsigned char key, int x, int y)
{
	keyboardState[key] = true;
}
void keyboardDownSpecial(int key, int x, int y)
{
	keyboardState[key] = true;
}
void keyboardUp(unsigned char key, int x, int y)
{
	keyboardState[key] = false;
}
void keyboardUpspecial(int key, int x, int y)
{
	keyboardState[key] = false;
}

void move()
{
	float cameraSpeed = 0.0035f;
	// 相机控制
	if (keyboardState['w']) camera.position += cameraSpeed * camera.direction;
	if (keyboardState['s']) camera.position -= cameraSpeed * camera.direction;
	if (keyboardState['a']) camera.position -= cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
	if (keyboardState['d']) camera.position += cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
	if (keyboardState[GLUT_KEY_CTRL_L]) camera.position.y -= cameraSpeed;
	if (keyboardState[' ']) camera.position.y += cameraSpeed;
	// 光源位置控制
	if (keyboardState[GLUT_KEY_RIGHT]) shadowCamera.position.x += cameraSpeed;
	if (keyboardState[GLUT_KEY_LEFT]) shadowCamera.position.x -= cameraSpeed;
	if (keyboardState[GLUT_KEY_UP]) shadowCamera.position.y += cameraSpeed;
	if (keyboardState[GLUT_KEY_DOWN]) shadowCamera.position.y -= cameraSpeed;
	glutPostRedisplay();    // 重绘
}
void init()
{
	// 生成着色器程序对象
	program = getShaderProgram("shaders/fshader.fsh", "shaders/vshader.vsh");
	shadowProgram = getShaderProgram("shaders/shadow.fsh", "shaders/shadow.vsh");
	debugProgram = getShaderProgram("shaders/debug.fsh", "shaders/debug.vsh");

	// ------------------------------------------------------------------------ // 

	// 读取 obj 模型
	/*Model tree1 = Model();
	tree1.translate = glm::vec3(-3, -1.1, 0);
	tree1.scale = glm::vec3(0.8,0.8, 0.8);
	tree1.load("models/nanosuit.obj");
	models.push_back(tree1);*/

	Model tree2 = Model();
	tree2.translate = glm::vec3(0, -10, 6);
	tree2.scale = glm::vec3(0.8, 0.8, 0.8);
	tree2.load("models/nanosuit.obj");
	models.push_back(tree2);

	Model plane = Model();
	plane.translate = glm::vec3(0, -1.1, 0);
	plane.scale = glm::vec3(40, 40, 40);
	plane.rotate = glm::vec3(0, 0, 0);
	plane.load("models/quad.obj");
	models.push_back(plane);

	// 光源位置标志物
	Model vlight = Model();
	vlight.translate = glm::vec3(1, 0, -1);
	vlight.rotate = glm::vec3(-90, 0, 0);
	vlight.scale = glm::vec3(13, 13, 13);
	vlight.load("models/Stanford Bunny.obj");
	models.push_back(vlight);

	// ------------------------------------------------------------------------ // 

	// 生成一个四方形做荧幕 -- 用以显示纹理中的数据
	Mesh msquare;
	msquare.vertexPosition = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0) };
	msquare.vertexTexCoord = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1) };
	msquare.index = { 0,1,2,2,1,3 };
	msquare.bindData();
	screen.meshes.push_back(msquare);

	// ------------------------------------------------------------------------ // 

	// 正交投影参数配置 -- 视界体范围 -- 调整到场景一般大小即可
	shadowCamera.left = -30;
	shadowCamera.right = 30;
	shadowCamera.bottom = -30;
	shadowCamera.top = 30;
	shadowCamera.position = glm::vec3(0, 4, 15);

	// 创建shadow帧缓冲
	glGenFramebuffers(1, &shadowMapFBO);
	// 创建阴影纹理
	glGenTextures(1, &shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 将阴影纹理绑定到 shadowMapFBO 帧缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ------------------------------------------------------------------------ // 

	glEnable(GL_DEPTH_TEST);  // 开启深度测试
	glClearColor(1.0, 1.0, 1.0, 1.0);   // 背景颜色
}

void display()
{

	move();

	models.back().translate = shadowCamera.position + glm::vec3(0, 0, 2);

	glUseProgram(shadowProgram);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, shadowMapResolution, shadowMapResolution);


	shadowCamera.direction = glm::normalize(glm::vec3(0,0,0) - shadowCamera.position);

	glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "view"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getViewMatrix(false)));
	glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "projection"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getProjectionMatrix(false)));

	for (auto m : models)
	{
		m.draw(shadowProgram);
	}

	glUseProgram(program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, windowWidth, windowHeight);

	// 传视图矩阵
	glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
	// 传投影矩阵
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

	//为什么你需要下面这个shadowvp呢,因为你不是在前面算完阴影贴图就完事了,在这里正常渲染的时候还要再变换一次来得到此时在光源视角
	//空间中的深度才能比较呀
	glm::mat4 shadowVP = shadowCamera.getProjectionMatrix(false) * shadowCamera.getViewMatrix(false);
	glUniformMatrix4fv(glGetUniformLocation(program, "shadowVP"), 1, GL_FALSE, glm::value_ptr(shadowVP));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glUniform1i(glGetUniformLocation(program, "shadowtex"), 1);

	glUniform3fv(glGetUniformLocation(program, "lightPos"), 1, glm::value_ptr(shadowCamera.position));
	glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, glm::value_ptr(camera.position));

	for (auto m : models)
	{
		//std::cout << m.meshes.size() << std::endl;
		m.draw(program);
	}

	glDisable(GL_DEPTH_TEST);
	glUseProgram(debugProgram);
	glViewport(0, 0, windowWidth / 3, windowHeight / 3);

	glActiveTexture(GL_TEXTURE1);
	GLuint test = models.back().textureMap.begin()->second;

	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glUniform1i(glGetUniformLocation(debugProgram, "shadowtex"), 1);
	
	// 绘制
	screen.draw(debugProgram);
	glEnable(GL_DEPTH_TEST);

	glutSwapBuffers();

}



int main(int argc, char** argv)
{
	glutInit(&argc, argv);              // glut初始化
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);// 窗口大小
	glutCreateWindow("8 - shadowMapping"); // 创建OpenGL上下文

#ifdef __APPLE__
#else
	glewInit();
#endif

	init();

	// 绑定鼠标移动函数 -- 
	//glutMotionFunc(mouse);  // 左键按下并且移动
	glutPassiveMotionFunc(mouse);   // 鼠标直接移动
	//glutMouseWheelFunc(mouseWheel); // 滚轮缩放

	// 绑定键盘函数
	glutKeyboardFunc(keyboardDown);
	glutSpecialFunc(keyboardDownSpecial);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialUpFunc(keyboardUpspecial);

	glutDisplayFunc(display);           // 设置显示回调函数 -- 每帧执行
	glutMainLoop();                     // 进入主循环

	return 0;
}
