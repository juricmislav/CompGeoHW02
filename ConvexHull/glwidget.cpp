// Convexe Hülle
// (c) Georg Umlauf, 2015
// Edited and developed for Computational Geometry Project by Mislav Jurić

#include "glwidget.h"
#include <QtGui>
#include <GL/glu.h>
#include "mainwindow.h"
#include <iostream>

// functions declaration
void drawGraham();
void drawJarvis();
void sweepLine();
void drawPolygon(std::vector<QPointF>);
void drawLines(std::map<float, QLineF>);
boolean isLeftTurn(std::vector<QPointF>);
QPointF findSmallestPoint();
QPointF findSmallestPointAngle(QPointF, QPointF, std::vector<QPointF>);
float calculateDeterminant(QPointF, QPointF, QPointF);
float calculateAngleDotProduct(QPointF, QPointF, QPointF);
void findLineIntersections(QLineF, std::map<float, QLineF>);
void reset();

// class variables
boolean grahamSelected = false;
boolean jarvisSelected = false;
boolean sweepLineSelected = false;
std::vector<QPointF> points = {};
std::vector<QLineF> intersectionLines = {};
std::vector<QPointF> intersectionPoints = {};

bool compareXCoordinate(QPointF a, QPointF b)
{
	if (a.x() == b.x()) return a.y() < b.y();
	return a.x() < b.x();
}

bool compareYCoordinateSmallest(QPointF a, QPointF b)
{
	if (a.y() == b.y()) return a.x() < b.x();
	return a.y() < b.y();
}

GLWidget::GLWidget(QWidget *parent) : QGLWidget(parent)
{
}

GLWidget::~GLWidget()
{
}

void GLWidget::paintGL()
{
	// clear
	glClear(GL_COLOR_BUFFER_BIT);

	// Koordinatensystem
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_LINES);
	glVertex2f(-1.0, 0.0);
	glVertex2f(1.0, 0.0);
	glVertex2f(0.0, -1.0);
	glVertex2f(0.0, 1.0);
	glEnd();

	if (points.size() > 0)
	{
		glColor3f(1, 0, 1);
		glBegin(GL_POINTS);
		for (auto &point : points)
		{
			glVertex2f(point.x(), point.y());
		}
		glEnd();
	}

	if (grahamSelected) drawGraham();
	if (jarvisSelected) drawJarvis();
	if (sweepLineSelected) sweepLine();
	
}

void GLWidget::initializeGL()
{
	resizeGL(width(), height());
	glPointSize(3);
	glEnable(GL_PROGRAM_POINT_SIZE);
}

void GLWidget::resizeGL(int width, int height)
{
	aspectx = 1.0;
	aspecty = 1.0;
	if (width>height) aspectx = float(width) / height;
	else              aspecty = float(height) / width;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-aspectx, aspectx, -aspecty, aspecty);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

QPointF GLWidget::transformPosition(QPoint p)
{
	return QPointF((2.0*p.x() / width() - 1.0)*aspectx,
		-(2.0*p.y() / height() - 1.0)*aspecty);
}

void GLWidget::keyPressEvent(QKeyEvent * event)
{
	switch (event->key()) {
	default:
		break;
	}
	update();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	QPointF posF = transformPosition(event->pos());
	if (event->buttons() & Qt::LeftButton) {
		if (!sweepLineSelected || (points.size() % 2 == 0))
		{
			points.push_back(posF);
			update();
			return;
		}
		QPointF lastPoint = points.at(points.size() - 1);
		float angle = calculateAngleDotProduct(lastPoint, QPointF(qreal(lastPoint.x() + 0.0000001), lastPoint.y()), posF);
		if (angle <= M_PI / 4 || angle >= M_PI * 3 / 4)
		{
			points.push_back(QPointF(posF.x(), lastPoint.y()));
			update();
			return;
		}
		points.push_back(QPointF(lastPoint.x(), posF.y()));
	}
	update();
}

void GLWidget::radioButton1Clicked()
{
	// Jarvis' March selected
	grahamSelected = false;
	jarvisSelected = true;
	sweepLineSelected = false;
	update();
}

void GLWidget::radioButton2Clicked()
{
	// Graham's Scan selected
	grahamSelected = true;
	jarvisSelected = false;
	sweepLineSelected = false;
	update();
}

void GLWidget::radioButton3Clicked()
{
	// Sweep Line selected
	reset();
	sweepLineSelected = true;
	update();
}

void GLWidget::radioButton4Clicked()
{
	// reset
	reset();
	update();
}

void reset()
{
	grahamSelected = false;
	jarvisSelected = false;
	sweepLineSelected = false;
	points.clear();
	intersectionLines.clear();
	intersectionPoints.clear();
}

void drawGraham()
{
	std::sort(points.begin(), points.end(), compareXCoordinate);

	std::vector<QPointF> upEdges = {};
	std::vector<QPointF> lowEdges = {};

	if (points.size() >= 3)
	{
		int pointSize = points.size();
		upEdges.push_back(points.at(0));
		upEdges.push_back(points.at(1));
		for (int i = 2; i < pointSize; ++i)
		{
			upEdges.push_back(points.at(i));
			while (upEdges.size() > 2 && isLeftTurn(upEdges))
			{
				upEdges.erase(upEdges.end() - 2);
			}
		}

		lowEdges.push_back(points.at(pointSize - 1));
		lowEdges.push_back(points.at(pointSize - 2));
		for (int i = pointSize - 3; i >= 0; --i)
		{
			lowEdges.push_back(points.at(i));
			while (lowEdges.size() > 2 && isLeftTurn(lowEdges))
			{
				lowEdges.erase(lowEdges.end() - 2);
			}
		}

		if (lowEdges.size() >= 2)
		{
			lowEdges.erase(lowEdges.end() - 1);
			lowEdges.erase(lowEdges.begin());
		}
		upEdges.insert(std::end(upEdges), std::begin(lowEdges), std::end(lowEdges));
	}

	drawPolygon(upEdges);
}

boolean isLeftTurn(std::vector<QPointF> points)
{
	int size = points.size();

	QPointF a = points.at(size - 3);
	QPointF b = points.at(size - 2);
	QPointF c = points.at(size - 1);

	return calculateDeterminant(a, b, c) > 0 ? true : false;
}

float calculateDeterminant(QPointF a, QPointF b, QPointF c)
{
	float ABx = b.x() - a.x();
	float ABy = b.y() - a.y();
	float ACx = c.x() - a.x();
	float ACy = c.y() - a.y();
	// cross product: AC x AB
	return ABx * ACy - ABy * ACx;
}

float calculateAngleDotProduct(QPointF a, QPointF b, QPointF c)
{
	float ABx = b.x() - a.x();
	float ABy = b.y() - a.y();
	float CBx = b.x() - c.x();
	float CBy = b.y() - c.y();

	// calculating dot product: [ABx, ABy] and [CBx, CBy]
	float dot = ABx * CBx + ABy * CBy;
	// calculating product of modules: |AB|*|BC|
	float module = sqrt(ABx * ABx + ABy * ABy) * sqrt(CBx * CBx + CBy * CBy);
	// calculating angle
	float cos = dot / module;

	return M_PI - acos(cos);
}

void drawJarvis()
{
	if (points.size() < 3) return;
	std::vector<QPointF> sortedPoints = {};
	for (QPointF &point : points)
	{
		sortedPoints.push_back(point);
	}
	std::sort(sortedPoints.begin(), sortedPoints.end(), compareYCoordinateSmallest);

	QPointF p = sortedPoints.at(0);
	std::vector<QPointF> edges = {};
	edges.push_back(p);

	QPointF q = findSmallestPointAngle(p, QPointF(qreal(p.x() + 0.0001), qreal(p.y())), sortedPoints);
	edges.push_back(q);
	sortedPoints.erase(std::remove(sortedPoints.begin(), sortedPoints.end(), q), sortedPoints.end());

	while (q != p)
	{
		q = findSmallestPointAngle(edges.at(edges.size() - 2), edges.at(edges.size() - 1), sortedPoints);
		edges.push_back(q);
	}

	if (edges.size() >= 3) drawPolygon(edges);
}

QPointF findSmallestPointAngle(QPointF startPoint, QPointF endPoint, std::vector<QPointF> sortedPoints)
{
	QPointF closestPoint;
	float smallestAngle = M_PI;
	for (QPointF &point : sortedPoints)
	{
		float angle = calculateAngleDotProduct(startPoint, endPoint, point);
		if (angle <= smallestAngle)
		{
			smallestAngle = angle;
			closestPoint = point;
		}
	}
	sortedPoints.erase(std::remove(sortedPoints.begin(), sortedPoints.end(), closestPoint), sortedPoints.end());
	return closestPoint;
}

void sweepLine()
{
	if (points.size() < 2) return;

	std::map<float, QLineF> lineSegments = {};
	for (int i = 0; i + 1 < points.size(); i = i + 2)
	{
		lineSegments[points.at(i).y()] = QLineF(points.at(i), points.at(i + 1));
	}
	drawLines(lineSegments);
	if (lineSegments.size() < 2 || points.size() % 2 != 0) return;

	// Q - set with all events sorted by x-coordinate
	std::vector<QPointF> Q = { points };
	std::sort(Q.begin(), Q.end(), compareXCoordinate);

	// list of active segmenets, sorted by y-coordinate
	std::map<float, QLineF> L = {};
	for (auto &p : Q)
	{
		if (lineSegments.find(p.y()) == lineSegments.end()) continue;
		QLineF line = lineSegments[p.y()];

		// p is left end point of horizontal segment
		if (line.x1() == p.x())
		{
			L[p.y()] = line;
		}

		// p is right end point of horizontal segment
		if (line.x2() == p.x())
		{
			L.erase(p.y());
		}

		// p is a part of vertical segment
		if (line.x1() == line.x2())
		{
			findLineIntersections(line, L);
		}
	}
	drawLines(lineSegments);
	
}

void findLineIntersections(QLineF verticalLine, std::map<float, QLineF> L)
{
	float y1 = verticalLine.y1();
	float y2 = verticalLine.y2();
	float x = verticalLine.x1();

	if (y1 > y2)
	{
		float temp = y1;
		y1 = y2;
		y2 = temp;
	}

	std::map<float, QLineF>::iterator low, upper;
	low = L.lower_bound(y1);
	upper = L.upper_bound(y2);
	
	bool hasIntersection = false;
	for (auto it = low; it != upper; ++it)
	{
		intersectionLines.push_back(it->second);
		intersectionPoints.push_back(QPointF(x, it->second.y1()));
		hasIntersection = true;
	}
	if (hasIntersection)
	{
		intersectionLines.push_back(verticalLine);
	}
}

void drawLines(std::map<float, QLineF> lineSegments)
{
	glColor3f(1, 1, 0);
	glBegin(GL_LINES);
	for (auto &x : lineSegments)
	{
		if (std::find(intersectionLines.begin(), intersectionLines.end(), x.second) != intersectionLines.end()) continue;
		glVertex2f(x.second.x1(), x.second.y1());
		glVertex2f(x.second.x2(), x.second.y2());
	}
	glEnd();

	if (!intersectionLines.empty())
	{
		glColor3f(1, 0, 0);
		glBegin(GL_LINES);
		for (auto &x : intersectionLines)
		{
			glVertex2f(x.x1(), x.y1());
			glVertex2f(x.x2(), x.y2());
		}
		glEnd();
	}

	if (!intersectionPoints.empty())
	{
		glColor3f(0, 0, 1);
		glBegin(GL_POINTS);
		for (auto &p : intersectionPoints)
		{
			glVertex2f(p.x(), p.y());
		}
		glEnd();
	}
}

void drawPolygon(std::vector<QPointF> edges)
{
	if (edges.size() >= 3)
	{
		glColor3f(1, 1, 0);
		glBegin(GL_LINE_LOOP);
		for (auto &point : edges)
		{
			glVertex2f(point.x(), point.y());
		}
		glEnd();
	}
}
