#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#define mapHight 15
#define mapWidth 15
#define windowHight 900
#define windowWidth 1800
#define pi 3.14159265359
#define deg2rad 0.0174532925
#define speed 0.05
#define rotateSpeed 2
#define mapScale 20
#define wallHight 900
#define maxVision 10

sf::RenderWindow window;
sf::Transformable cam;
sf::RectangleShape mapGrid[mapHight][mapWidth];
sf::RectangleShape ground;
sf::Font font;
sf::Text txtData;
sf::Texture camTexture;
sf::Sprite camSprite;
sf::CircleShape spot;

void processInput();
void render();
void topViewInit();
void topViewDraw();
void spotPosition();
void raycasting(int column);
sf::Color shade(sf::Color original, float length);

extern int map[mapHight][mapWidth];

sf::Color colorSet[3] =
{
	sf::Color(240, 248, 255),
	sf::Color(139, 69, 19),
	sf::Color(70, 130, 180)
};

int main()
{
	//視窗建立，防鋸齒、垂直同步、限制幀率
	sf::ContextSettings settings;
	settings.antialiasingLevel = 10;
	window.create(sf::VideoMode(windowWidth, windowHight), "Raycast Test", sf::Style::Close, settings);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	//相機初始化
	cam.setPosition(sf::Vector2f(5, 5));
	cam.setRotation(0);

	//地板初始化
	ground.setFillColor(sf::Color(128, 128, 128));
	ground.setPosition(0, windowHight / 2);
	ground.setSize(sf::Vector2f(windowWidth, windowHight / 2));

	//cam數據字串初始化
	font.loadFromFile("arial.ttf");
	txtData.setFont(font);
	txtData.setCharacterSize(30);
	txtData.setPosition(windowWidth - 200, 50);
	txtData.setFillColor(sf::Color::White);

	//俯視角初始化
	topViewInit();

	//遊戲循環
	while (window.isOpen()) {
		processInput();
		render();
	}

	return 0;
}

void processInput()
{
	//關閉視窗事件
	sf::Event event;
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::Closed)
			window.close();
	}

	//鍵盤控制
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
		cam.move(cos(cam.getRotation() * deg2rad) * speed, sin(cam.getRotation() * deg2rad) * speed);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
		cam.move(sin(cam.getRotation() * deg2rad) * speed, cos(cam.getRotation() * deg2rad) * -speed);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
		cam.move(cos(cam.getRotation() * deg2rad) * -speed, sin(cam.getRotation() * deg2rad) * -speed);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
		cam.move(sin(cam.getRotation() * deg2rad) * -speed, cos(cam.getRotation() * deg2rad) * speed);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) {
		cam.rotate(-rotateSpeed);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) {
		cam.rotate(rotateSpeed);
	}
}

void render()
{
	//清除上一幀
	window.clear(sf::Color(16, 16, 16));

	//繪製地面
	window.draw(ground);

	//繪製牆面
	for (int i = 0; i < windowWidth; i++)
	{
		raycasting(i);
	}

	//繪製數據
	std::string strData;
	strData = "x: " + std::to_string(cam.getPosition().x) + "\n";
	strData += "y: " + std::to_string(cam.getPosition().y) + "\n";
	strData += "angle: " + std::to_string((int)cam.getRotation());

	txtData.setString(strData);
	window.draw(txtData);

	//繪製俯視角
	topViewDraw();

	//顯示畫面
	window.display();
}

void topViewInit()
{
	//mapGrid初始化
	for (int i = 0; i < mapHight; i++)
	{
		for (int j = 0; j < mapWidth; j++)
		{
			mapGrid[i][j].setPosition(sf::Vector2f(j * mapScale, i * mapScale));
			mapGrid[i][j].setSize(sf::Vector2f(mapScale, mapScale));
			mapGrid[i][j].setOutlineColor(sf::Color::Black);
			mapGrid[i][j].setOutlineThickness(1);
			mapGrid[i][j].setFillColor(colorSet[map[i][j]]);
		}
	}

	//sprite和texture初始化
	camTexture.loadFromFile("arrow.png");
	camTexture.setSmooth(true);
	camSprite.setTexture(camTexture);
	camSprite.setOrigin(20, 20);
	camSprite.setScale((float)mapScale / 60, (float)mapScale / 60);

	//交點初始化
	spot.setRadius(mapScale / 4);
	spot.setOrigin(mapScale / 4, mapScale / 4);
}

void topViewDraw()
{
	//繪製mapGrid
	for (int i = 0; i < mapHight; i++)
	{
		for (int j = 0; j < mapWidth; j++)
		{
			window.draw(mapGrid[i][j]);
		}
	}

	//繪製sprite
	camSprite.setPosition(cam.getPosition().x * mapScale, cam.getPosition().y * mapScale);
	camSprite.setRotation(cam.getRotation());
	window.draw(camSprite);

	//繪製交點
	spotPosition();
	window.draw(spot);
}

void spotPosition()
{
	//計算射線角度
	float rayDirection = cam.getRotation() * deg2rad;

	//前置計算
	sf::Vector2f quadrant(cos(rayDirection) >= 0 ? 1 : -1, sin(rayDirection) >= 0 ? 1 : -1);
	sf::Vector2f tilePosition(floor(cam.getPosition().x), floor(cam.getPosition().y));
	sf::Vector2f delta = cam.getPosition() - tilePosition;
	delta.x = quadrant.x == 1 ? 1 - delta.x : -delta.x;
	delta.y = quadrant.y == 1 ? 1 - delta.y : -delta.y;
	float minLength = 10000;

	//鉛垂線交點計算
	sf::Vector2f vIntersection(cam.getPosition().x + delta.x, cam.getPosition().y + delta.x * tan(rayDirection));
	sf::Vector2f vStep(quadrant.x, quadrant.x * tan(rayDirection));

	while (vIntersection.x >= 0 && vIntersection.x <= mapWidth && vIntersection.y >= 0 && vIntersection.y <= mapHight)
	{
		if (map[(int)floor(vIntersection.y)][(int)round(vIntersection.x) - (quadrant.x == -1 ? 1 : 0)] != 0) {
			float newLength = sqrt(pow(vIntersection.x - cam.getPosition().x, 2) + pow(vIntersection.y - cam.getPosition().y, 2));
			if (newLength < minLength) {
				minLength = newLength;
				spot.setFillColor(sf::Color::Red);
				spot.setPosition(vIntersection * (float)mapScale);
			}
			break;
		}
		vIntersection += vStep;
	}

	//水平線交點計算
	sf::Vector2f hIntersection(cam.getPosition().x + delta.y / tan(rayDirection), cam.getPosition().y + delta.y);
	sf::Vector2f hStep(quadrant.y / tan(rayDirection), quadrant.y);

	while (hIntersection.x >= 0 && hIntersection.x <= mapWidth && hIntersection.y >= 0 && hIntersection.y <= mapHight)
	{
		if (map[(int)round(hIntersection.y) - (quadrant.y == -1 ? 1 : 0)][(int)floor(hIntersection.x)] != 0) {
			float newLength = sqrt(pow(hIntersection.x - cam.getPosition().x, 2) + pow(hIntersection.y - cam.getPosition().y, 2));
			if (newLength < minLength) {
				minLength = newLength;
				spot.setFillColor(sf::Color::Green);
				spot.setPosition(hIntersection * (float)mapScale);
			}
			break;
		}
		hIntersection += hStep;
	}
}

void raycasting(int column)
{
	//計算射線角度
	float angleBetween = atan2(column - windowWidth / 2, windowWidth / 2);
	float rayDirection = cam.getRotation() * deg2rad + angleBetween;

	//前置計算
	sf::Vector2f quadrant(cos(rayDirection) >= 0 ? 1 : -1, sin(rayDirection) >= 0 ? 1 : -1);
	sf::Vector2f tilePosition(floor(cam.getPosition().x), floor(cam.getPosition().y));
	sf::Vector2f delta = cam.getPosition() - tilePosition;
	delta.x = quadrant.x == 1 ? 1 - delta.x : -delta.x;
	delta.y = quadrant.y == 1 ? 1 - delta.y : -delta.y;
	sf::Vector2f intersection;
	sf::Vector2f step;
	float rayLength = maxVision;
	int tag = 0;

	//鉛垂線交點計算
	intersection = sf::Vector2f(cam.getPosition().x + delta.x, cam.getPosition().y + delta.x * tan(rayDirection));
	step = sf::Vector2f(quadrant.x, quadrant.x * tan(rayDirection));

	while (intersection.x >= 0 && intersection.x <= mapWidth && intersection.y >= 0 && intersection.y <= mapHight)
	{
		if (map[(int)floor(intersection.y)][(int)round(intersection.x) - (quadrant.x == -1 ? 1 : 0)] != 0) {
			float newLength = sqrt(pow(intersection.x - cam.getPosition().x, 2) + pow(intersection.y - cam.getPosition().y, 2));
			if (newLength < rayLength) {
				tag = map[(int)floor(intersection.y)][(int)round(intersection.x) - (quadrant.x == -1 ? 1 : 0)];
				rayLength = newLength;
			}
			break;
		}
		intersection += step;
	}

	//水平線交點計算
	intersection = sf::Vector2f(cam.getPosition().x + delta.y / tan(rayDirection), cam.getPosition().y + delta.y);
	step = sf::Vector2f(quadrant.y / tan(rayDirection), quadrant.y);

	while (intersection.x >= 0 && intersection.x <= mapWidth && intersection.y >= 0 && intersection.y <= mapHight)
	{
		if (map[(int)round(intersection.y) - (quadrant.y == -1 ? 1 : 0)][(int)floor(intersection.x)] != 0) {
			float newLength = sqrt(pow(intersection.x - cam.getPosition().x, 2) + pow(intersection.y - cam.getPosition().y, 2));
			if (newLength < rayLength) {
				tag = map[(int)round(intersection.y) - (quadrant.y == -1 ? 1 : 0)][(int)floor(intersection.x)];
				rayLength = newLength;
			}
			break;
		}
		intersection += step;
	}

	//消除魚眼效果
	float planeLength = rayLength * cos(angleBetween);

	//繪製牆面
	float columnHight = wallHight / planeLength;
	float columnOffset = (windowHight - columnHight) / 2;

	sf::RectangleShape wallCloumn;
	wallCloumn.setFillColor(shade(colorSet[tag], rayLength));
	wallCloumn.setPosition(column, columnOffset);
	wallCloumn.setSize(sf::Vector2f(1, columnHight));

	window.draw(wallCloumn);
}

sf::Color shade(sf::Color color, float length)
{
	float max = fmax(fmax(color.r, color.g), color.b);
	float min = fmin(fmin(color.r, color.g), color.b);

	float h;
	float s;
	float v;

	//計算h
	if (max == min) {
		h = 0;
	}
	else if (max == color.r) {
		h = 60 * (color.g - color.b) / (max - min) + (color.g >= color.b ? 0 : 360);
	}
	else if (max == color.g) {
		h = 60 * (color.b - color.r) / (max - min) + 120;
	}
	else {
		h = 60 * (color.r - color.g) / (max - min) + 240;
	}

	//計算s
	s = max == 0 ? 0 : ((max - min) / max);

	//計算v
	v = max;

	//v對距離作調整
	//v *= sqrt(1 - pow(length / maxVision, 2)); 
	//v *= 1 - (length / maxVision);
	v *= 1 - pow(length / maxVision, 4);

	//換回rgb
	h = h / 60;
	int i = (int)h;
	float c = h - i;

	float x = v * (1 - s);
	float y = v * (1 - s * c);
	float z = v * (1 - s * (1 - c));

	switch (i)
	{
	case 0:color.r = v; color.g = z; color.b = x; break;
	case 1:color.r = y; color.g = v; color.b = x; break;
	case 2:color.r = x; color.g = v; color.b = z; break;
	case 3:color.r = x; color.g = y; color.b = v; break;
	case 4:color.r = z; color.g = x; color.b = v; break;
	case 5:color.r = v; color.g = x; color.b = y; break;
	}

	return color;
}