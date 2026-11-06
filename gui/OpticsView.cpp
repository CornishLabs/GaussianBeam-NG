/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>
   Modified 2026 by Daniel Ruttley: ported to Qt 5 and Windows; added wavefront drawing; dark mode and new icons; better beam colours (see CHANGES.md)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "gui/OpticsView.h"
#include "gui/OpticsWidgets.h"
#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"
#include "gui/Theme.h"
#include "src/GaussianBeam.h"
#include "src/Utils.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <QtDebug>
#include <QSettings>

#include <cmath>

namespace
{
	/// Piecewise Gaussian used by the CIE colour matching function fit
	double cieLobe(double x, double mu, double sigmaLow, double sigmaHigh)
	{
		const double t = (x - mu)/(x < mu ? sigmaLow : sigmaHigh);
		return exp(-0.5*t*t);
	}

	/// sRGB transfer curve: linear intensity in [0, 1] to display value
	double srgbEncode(double v)
	{
		return (v <= 0.0031308) ? 12.92*v : 1.055*pow(v, 1./2.4) - 0.055;
	}
}

/**
* Colour of a beam of the given wavelength (in metres).
*
* The hue is how the eye sees the wavelength: the CIE 1931 colour matching
* functions, using the analytic fit of Wyman, Sloan and Shirley, "Simple
* Analytic Approximations to the CIE XYZ Color Matching Functions", Journal of
* Computer Graphics Techniques 2(2), 2013. XYZ is converted to linear sRGB;
* spectral colours are outside the sRGB gamut, so negative channels are clipped.
* The colour is scaled to full brightness, then gamma encoded.
*
* The fit is only used between 410 and 660 nm: outside this range the true hue
* hardly changes (violet below, red above) and the fit's tails are inaccurate.
* Invisible beams fade to half brightness: from 700 nm to 1100 nm in the
* infrared, and from 410 nm to 300 nm in the ultraviolet.
*/
QColor wavelengthColor(double wavelength)
{
	const double nm = wavelength*1e9;
	const double l = qBound(410., nm, 660.);

	// CIE 1931 2° colour matching functions
	const double X = 1.056*cieLobe(l, 599.8, 37.9, 31.0) + 0.362*cieLobe(l, 442.0, 16.0, 26.7)
	               - 0.065*cieLobe(l, 501.1, 20.4, 26.2);
	const double Y = 0.821*cieLobe(l, 568.8, 46.9, 40.5) + 0.286*cieLobe(l, 530.9, 16.3, 31.1);
	const double Z = 1.217*cieLobe(l, 437.0, 11.8, 36.0) + 0.681*cieLobe(l, 459.0, 26.0, 13.8);

	// Linear sRGB (D65), clipped to the displayable gamut
	double r = qMax(0.,  3.2406*X - 1.5372*Y - 0.4986*Z);
	double g = qMax(0., -0.9689*X + 1.8758*Y + 0.0415*Z);
	double b = qMax(0.,  0.0557*X - 0.2040*Y + 1.0570*Z);

	// Full brightness: only the hue comes from the wavelength
	const double maximum = qMax(r, qMax(g, b));
	r /= maximum;
	g /= maximum;
	b /= maximum;

	// Fade invisible wavelengths to half brightness
	double brightness = 1.;
	if (nm > 700.)
		brightness = 1. - 0.5*qMin(1., (nm - 700.)/400.);
	else if (nm < 410.)
		brightness = 1. - 0.5*qMin(1., (410. - nm)/110.);

	return QColor::fromRgbF(brightness*srgbEncode(r), brightness*srgbEncode(g), brightness*srgbEncode(b));
}

/////////////////////////////////////////////////
// OpticsScene class

OpticsScene::OpticsScene(OpticsBench& bench, QObject* parent)
	: QGraphicsScene(parent)
	, OpticsBenchNotify(bench)
{
	setItemIndexMethod(QGraphicsScene::NoIndex);

	m_targetBeamItem = new BeamItem(m_bench.targetBeam());
	m_targetBeamItem->setPlainStyle(false);
	m_targetBeamItem->setAuxiliary(true);
	m_targetBeamItem->setPos(0., 0.);
	addItem(m_targetBeamItem);

	m_showWavefronts = QSettings().value("OpticsScene/showWavefronts", true).toBool();

	m_bench.registerNotify(this);
}

void OpticsScene::showTargetBeam(bool show)
{
	m_targetBeamItem->setVisible(show);
}

void OpticsScene::showWavefronts(bool show)
{
	m_showWavefronts = show;
	QSettings().setValue("OpticsScene/showWavefronts", show);
	// Ask each beam to repaint itself
	foreach (BeamItem* beamItem, m_beamItems)
		beamItem->update();
}

bool OpticsScene::targetBeamVisible()
{
	return m_targetBeamItem->isVisible();
}

/// @todo get rid of this trick
#define SCENEHALFHEIGHT 0.005

void OpticsScene::OpticsBenchDataChanged(int startOptics, int endOptics)
{
	foreach (QGraphicsItem* graphicsItem, items())
	{
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
		{
			int opticsIndex = m_bench.opticsIndex(opticsItem->optics());
			if ((opticsIndex >= startOptics) && (opticsIndex <= endOptics))
			{
				opticsItem->setUpdate(false);
				opticsItem->setPos(opticsItem->optics()->position(), 0.);
				opticsItem->setUpdate(true);
			}
		}
	}

	for (int i = qMax(0, startOptics-1); i <= endOptics; i++)
	{
		const Optics* optics = m_bench.optics(i);
		m_beamItems[i]->setPos(0., 0.);
		if (i == 0)
			m_beamItems[i]->setLeftBound(m_bench.leftBoundary());
		else
			m_beamItems[i]->setLeftBound(optics->position() + optics->width());
		if (i == m_bench.nOptics()-1)
			m_beamItems[i]->setRightBound(m_bench.rightBoundary());
		else
			m_beamItems[i]->setRightBound(m_bench.optics(i+1)->position());
	}
}

void OpticsScene::OpticsBenchTargetBeamChanged()
{
	m_targetBeamItem->update();
}

void OpticsScene::OpticsBenchBoundariesChanged()
{
	qDebug() << "OpticsScene::OpticsBenchBoundariesChanged" << m_bench.nOptics();

	setSceneRect(m_bench.leftBoundary(), -SCENEHALFHEIGHT, m_bench.rightBoundary() - m_bench.leftBoundary(), 2.*SCENEHALFHEIGHT);

	m_targetBeamItem->setLeftBound(m_bench.leftBoundary());
	m_targetBeamItem->setRightBound(m_bench.rightBoundary());

	if ((m_beamItems.size() != 0) && (m_beamItems.size() == m_bench.nOptics()))
	{
		m_beamItems[0]->setLeftBound(m_bench.leftBoundary());
		m_beamItems[m_bench.nOptics()-1]->setRightBound(m_bench.rightBoundary());
	}

	foreach (QGraphicsView* view, views())
	{
		dynamic_cast<OpticsView*>(view)->adjustRange();
	}
}

void OpticsScene::OpticsBenchOpticsAdded(int index)
{
	OpticsItem* opticsItem = new OpticsItem(m_bench.optics(index), m_bench);
	addItem(opticsItem);

	BeamItem* beamItem = new BeamItem(m_bench.beam(index));
	m_beamItems.insert(index, beamItem);
	addItem(beamItem);
}

void OpticsScene::OpticsBenchOpticsRemoved(int index, int count)
{
	foreach (QGraphicsItem* graphicsItem, items())
		if (OpticsItem* opticsItem = dynamic_cast<OpticsItem*>(graphicsItem))
			if (m_bench.opticsIndex(opticsItem->optics()) == -1)
				removeItem(graphicsItem);

	for (int i = index + count - 1; i >= index; i--)
		removeItem(m_beamItems.takeAt(i));
}

void OpticsScene::addFitPoint(double position, double radius, QRgb color)
{
	QGraphicsEllipseItem* fitItem = new QGraphicsEllipseItem(-2., -2., 4., 4.);
	fitItem->setFlags(fitItem->flags() | QGraphicsItem::ItemIgnoresTransformations);
	fitItem->setPen(QPen(color));
	fitItem->setBrush(QBrush(color));
	fitItem->setPos(position, radius);
	fitItem->setZValue(2.);
	m_fitItems.push_back(fitItem);
	addItem(fitItem);
}

void OpticsScene::OpticsBenchFitsRemoved(int index, int count)
{
	Q_UNUSED(count);
	/// @todo with more houskeeping in OpticsScene::OpticsBenchFitDataChanged this will not work any more
	OpticsBenchFitDataChanged(index);
}

void OpticsScene::OpticsBenchFitDataChanged(int index)
{
	Q_UNUSED(index);

	/// @todo with a little bit of housekeeping, we could avoid recontructing all the items each time...
	while (!m_fitItems.isEmpty())
	{
		removeItem(m_fitItems.last());
		m_fitItems.removeLast();
	}

	for (int index = 0; index < m_bench.nFit(); index++)
	{
		Fit& fit = m_bench.fit(index);
		for (int i = 0; i < fit.size(); i++)
			if (fit.value(i) != 0.)
			{
				addFitPoint(fit.position(i), -fit.radius(i), fit.color());
				addFitPoint(fit.position(i),  fit.radius(i), fit.color());
			}
	}
}

/////////////////////////////////////////////////
// OpticsView class

OpticsView::OpticsView(QGraphicsScene* scene)
	: QGraphicsView(scene)
{
	setRenderHint(QPainter::Antialiasing);
	setBackgroundBrush(Theme::viewBackground());
	connect(Theme::instance(), SIGNAL(changed()), this, SLOT(themeChanged()));
	m_horizontalRange = 0.;
	m_verticalRange = 0.;
	m_statusWidget = 0;

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_horizontalRuller = new RullerSlider(this);
	m_verticalRuller = new RullerSlider(this, true);
	setHorizontalScrollBar(m_horizontalRuller);
	setVerticalScrollBar(m_verticalRuller);

	m_opticsViewProperties = new OpticsViewProperties(this);
	m_opticsViewProperties->hide();

	CornerWidget* cornerWidget = new CornerWidget("zoom-fit", m_opticsViewProperties, this, true);
	cornerWidget->setToolTip(tr("View properties"));
	setCornerWidget(cornerWidget);

	setResizeAnchor(QGraphicsView::AnchorViewCenter);
	centerOn(0., 0.);

	connect(m_horizontalRuller, SIGNAL(valueChanged(int)), this, SLOT(scrollUpdated(int)));
}

void OpticsView::adjustRange()
{
	if ((m_horizontalRange > scene()->width()) || (m_horizontalRange == 0.))
		m_horizontalRange = scene()->width();

	if ((m_verticalRange > scene()->height()) || (m_verticalRange == 0.))
		m_verticalRange = scene()->height();

	if ((m_horizontalRange == 0.) || (m_verticalRange == 0.) || (width() == 0.) || (height() == 0.))
		return;

	QMatrix scaling = matrix();
	scaling.setMatrix(width()/m_horizontalRange, scaling.m12(), scaling.m21(), height()/m_verticalRange, scaling.dx(), scaling.dy());
	setMatrix(scaling);

	m_opticsViewProperties->setViewWidth(m_horizontalRange);
	m_opticsViewProperties->setViewHeight(m_verticalRange);
	m_opticsViewProperties->setViewOrigin(origin());
}

void OpticsView::showProperties(bool show)
{
	m_opticsViewProperties->setVisible(show);
}

bool OpticsView::propertiesVisible()
{
	return m_opticsViewProperties->isVisible();
}

/// Switch the background, and repaint the optics and labels, in the new light/dark colours
void OpticsView::themeChanged()
{
	setBackgroundBrush(Theme::viewBackground());
	viewport()->update();
}

void OpticsView::scrollUpdated(int value)
{
	Q_UNUSED(value);
	m_opticsViewProperties->setViewOrigin(origin());
}

double OpticsView::origin()
{
	return mapToScene(viewport()->rect().topLeft()).x();
}

void OpticsView::setOrigin(double origin)
{
	if (origin == OpticsView::origin())
		return;

	if (origin < sceneRect().left())
		origin = sceneRect().left();

	if (origin > sceneRect().right() - m_horizontalRange)
		origin = sceneRect().right() - m_horizontalRange;

	int value = int(origin*m_horizontalRuller->rullerScale());
	horizontalScrollBar()->setValue(value);
	m_opticsViewProperties->setViewOrigin(origin);
}

void OpticsView::setHorizontalRange(double horizontalRange)
{
	m_horizontalRange = horizontalRange;
	adjustRange();
}

void OpticsView::setVerticalRange(double verticalRange)
{
	m_verticalRange = verticalRange;
	adjustRange();
}

void OpticsView::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	adjustRange();
	m_opticsViewProperties->move(viewport()->width() - m_opticsViewProperties->width(), viewport()->height() - m_opticsViewProperties->height());
}

void OpticsView::wheelEvent(QWheelEvent* event)
{
	/// @todo this does not work
	QApplication::sendEvent(m_horizontalRuller, event);
}

void OpticsView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_statusWidget)
	{
		QPointF position = mapToScene(event->pos());
		position.setY(0.);

		foreach (QGraphicsItem* graphicsItem, items())
			if (BeamItem* beamItem = dynamic_cast<BeamItem*>(graphicsItem))
				if (beamItem->boundingRect().contains(position) && !beamItem->auxiliary())
				{
					m_statusWidget->showBeamInfo(beamItem->beam(), position.x());
					break;
				}
	}

	QGraphicsView::mouseMoveEvent(event);
}

void OpticsView::drawBackground(QPainter* painter, const QRectF& rect)
{
	// Fill with the background brush (white)
	QGraphicsView::drawBackground(painter, rect);

/*	/// @todo if drawing a grid and rullers, set background cache
	const double pixel = scene()->width()/width();

	QBrush backgroundBrush(Qt::white);
	painter->fillRect(rect, backgroundBrush);

	QPen opticalAxisPen(Qt::lightGray, 2.*pixel, Qt::DashDotLine, Qt::FlatCap, Qt::RoundJoin);
	painter->setPen(opticalAxisPen);
	/// @todo replace scene()->sceneRect().left() by rect.left() once dashOffset works
	painter->drawLine(QPointF(scene()->sceneRect().left(), 0.), QPointF(rect.right(), 0.));


	QPen verticalAxisPen(Qt::lightGray, pixel, Qt::DotLine, Qt::FlatCap, Qt::RoundJoin);
	/// @todo idem
	painter->setPen(verticalAxisPen);
	painter->drawLine(QPointF(0., scene()->sceneRect().top()), QPointF(0., rect.bottom()));*/
}

/////////////////////////////////////////////////
// OpticsItem class

OpticsItem::OpticsItem(const Optics* optics, OpticsBench& bench)
	: QGraphicsItem()
	, m_optics(optics)
	, m_bench(bench)
{
	if (m_optics->type() != CreateBeamType)
	{
		setCursor(Qt::OpenHandCursor);
		setFlag(ItemIsMovable);
#if QT_VERSION >= 0x040600
		// Needed for itemChange(ItemPositionChange) to be called, which keeps
		// the optics on the axis and updates the bench while dragging
		setFlag(ItemSendsGeometryChanges);
#endif
		setZValue(1);
	}
	else
		setZValue(-1);

	if (m_optics->width() == 0.)
		setFlag(QGraphicsItem::ItemIgnoresTransformations);

	m_update = true;
}

QRectF OpticsItem::boundingRect() const
{
	QRectF bounding;

	if (m_optics->width() == 0.)
		bounding = QRectF(QPointF(-10., -66.), QSizeF(2.*10., 2.*66.));
	else
		bounding = QRectF(QPointF(0., -SCENEHALFHEIGHT), QSizeF(m_optics->width(), 2.*SCENEHALFHEIGHT));

	return bounding;
}

QVariant OpticsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
	if ((change == ItemPositionChange && scene()) && m_update)
	{
		QPointF newPos = value.toPointF();
		QRectF rect = scene()->sceneRect();
		if (!rect.contains(newPos))
			newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
		newPos.setY(0.);
		// Propose the new position
		m_bench.setOpticsPosition(m_bench.opticsIndex(m_optics), newPos.x());
		// Adjust the new position to what the bench decided in last
		newPos.setX(m_optics->position());
		return newPos;
	}

	return QGraphicsItem::itemChange(change, value);
 }

void OpticsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	const QColor foreground = Theme::viewForeground();
	QBrush opticsBrush(Theme::opticsFill());
	painter->setBrush(opticsBrush);
	// Cosmetic pens are always 1 pixel wide. Since Qt 5, pens are not cosmetic by
	// default and would be 1 scene unit (1 m) wide, covering the whole view.
	QPen outlinePen(foreground);
	outlinePen.setCosmetic(true);
	painter->setPen(outlinePen);
	QPen textPen(foreground);
	textPen.setCosmetic(true);

	QPainterPath path;
	QRectF rect = boundingRect();
	QRectF rightRect = rect;
	rightRect.moveLeft(rect.width()/4.);
	QRectF leftRect = rect;
	leftRect.moveRight(-rect.width()/4.);

	if (m_optics->type() == CreateBeamType)
	{
		// no painting !
	}
	else if (m_optics->type() == LensType)
	{
		if (dynamic_cast<const Lens*>(m_optics)->focal() >= 0.)
		{
			path.moveTo(0., rect.top());
			path.arcTo(rect, 90., 180.);
			path.arcTo(rect, 270., 180.);
		}
		else
		{
			path.moveTo(leftRect.center().x(), rect.top());
			path.arcTo(rightRect, 90., 180.);
			path.arcTo(leftRect, 270., 180.);
		}
		painter->drawPath(path);
	}
	else if ((m_optics->type() == FlatInterfaceType) || (m_optics->type() == CurvedInterfaceType))
	{
		double indexRatio = dynamic_cast<const Interface*>(m_optics)->indexRatio();
		double origin = (indexRatio >= 1.) ? rect.right() : rect.left();
		path.moveTo(origin, rect.top());
		path.lineTo(rect.center().x(), rect.top());
		if (m_optics->type() == FlatInterfaceType)
			path.lineTo(rect.center().x(), rect.bottom());
		else
			path.arcTo(rect, 90., dynamic_cast<const CurvedInterface*>(m_optics)->surfaceRadius() >= 0. ? 180. : -180.);
		path.lineTo(origin, rect.bottom());
		painter->drawPath(path);
	}
	else if (m_optics->type() == DielectricSlabType)
	{
		painter->drawRect(rect);
	}
	else if (m_optics->type() == FlatMirrorType)
	{
		painter->drawRect(rect);
		QBrush ABCDBrush(foreground, Qt::BDiagPattern);
		painter->setBrush(ABCDBrush);
		painter->drawRect(rect);

	}
	else if (m_optics->type() == CurvedMirrorType)
	{
		painter->drawRect(rect);
		QBrush ABCDBrush(foreground, Qt::BDiagPattern);
		painter->setBrush(ABCDBrush);
		painter->drawRect(rect);
	}
	else if (m_optics->type() == GenericABCDType)
	{
		painter->setBrush(QBrush(Theme::abcdFill()));
		painter->drawRect(rect);
	}
/*
	if (m_optics->type() != CreateBeamType)
	{
		painter->setPen(textPen);
		QString text = QString::fromUtf8(m_optics->name().c_str());
		QRectF textRect(0., 0., 0., 0.);
		textRect.moveCenter(rect.center() - QPointF(0., rect.height()*0.7));
		textRect = painter->boundingRect(textRect, Qt::AlignCenter, text);
		painter->drawText(textRect, Qt::AlignCenter, text);
	}
*/
}

/////////////////////////////////////////////////
// BeamView class

BeamItem::BeamItem(const Beam* beam)
	: QGraphicsItem()
	, m_beam(beam)
{
	m_drawText = true;
	m_style = true;
	m_auxiliary = false;
	setZValue(0.);
}

void BeamItem::setLeftBound(double leftBound)
{
	prepareGeometryChange();
	m_leftBound = leftBound;
}

void BeamItem::setRightBound(double rightBound)
{
	prepareGeometryChange();
	m_rightBound = rightBound;
}

QRectF BeamItem::boundingRect() const
{
	QRectF result = scene()->sceneRect();
	result.setLeft(m_leftBound);
	result.setRight(m_rightBound);
	return result;
}

void BeamItem::drawSegment(double start, double stop, double pixel, int nStep, QPolygonF& polygon) const
{
	if (start < m_leftBound)
		start = m_leftBound;

	if (stop > m_rightBound)
		stop = m_rightBound;

	if (start >= stop)
		return;

	// "pixel" is added to avoid overlong iterations due to very small steps caused by rounding errors
	double step = (stop - start)/double(nStep) + pixel;
	if (step < pixel)
		step = pixel;

	for (double z = start; z <= stop; z += step)
		polygon.append(QPointF(z, m_beam->radius(z)));
}

/*
 * Wavefronts
 *
 * A Gaussian beam's wavefronts (surfaces of constant phase) are spherical with
 * radius of curvature R(z) = (z - zw) [1 + (z0/(z - zw))^2]. To scale, their
 * curvature is invisible in this view: the sag across the beam is w^2/(2R),
 * typically well under a micrometre, while one pixel horizontally is a
 * fraction of a millimetre.
 *
 * Instead, each wavefront is drawn as the curve that is perpendicular, on
 * screen, to the beam's rays y = c w(z). In the real (unstretched) geometry the
 * wavefronts are exactly perpendicular to these rays, so this is the true
 * wavefront as it would look if the view's stretching were applied to it as
 * well: its sag is the real sag multiplied by the square of the view's
 * horizontal/vertical scale ratio. It is flat at the waist, most curved near
 * the Rayleigh range, and becomes an arc centred on the waist in the far field.
 */

/// Trace the wavefront crossing the axis at @p position, from the lower to the upper edge of the beam.
/// @p horizontalScale and @p verticalScale are the view scales in m/pixel.
QPolygonF BeamItem::wavefront(double position, double horizontalScale, double verticalScale) const
{
	const double h = fabs(horizontalScale);
	const double v = fabs(verticalScale);
	const double zw = m_beam->waistPosition();
	const double z0 = m_beam->rayleigh();

	// Slope of the ray through (z, y), in pixels per pixel. Rays are y = c w(z), so
	// dy/dz = y w'(z)/w(z) = y (z - zw)/((z - zw)^2 + z0^2) = y/R(z), finite at the waist.
	struct Local
	{
		static double raySlope(double z, double y, double zw, double z0, double h, double v)
			{ return (h/v)*y*(z - zw)/(sqr(z - zw) + sqr(z0)); }
	};

	// Walk from the axis towards the upper edge of the beam in steps of one pixel,
	// always perpendicular (on screen) to the local ray. Midpoint (2nd order) method.
	QPolygonF upper;
	double z = position, y = 0.;
	upper << QPointF(z, y);
	for (int step = 0; step < 5000; step++)
	{
		double m = Local::raySlope(z, y, zw, z0, h, v);
		double n = sqrt(sqr(m) + 1.);
		double zMid = z - 0.5*h*m/n;
		double yMid = y + 0.5*v/n;
		m = Local::raySlope(zMid, yMid, zw, z0, h, v);
		n = sqrt(sqr(m) + 1.);
		double zNext = z - h*m/n;
		double yNext = y + v/n;

		// Stop at the edge of the beam, interpolating onto it
		double inside = y - m_beam->radius(z);             // < 0
		double outside = yNext - m_beam->radius(zNext);
		if (outside >= 0.)
		{
			double t = inside/(inside - outside);
			upper << QPointF(z + t*(zNext - z), y + t*(yNext - y));
			break;
		}

		z = zNext;
		y = yNext;
		upper << QPointF(z, y);
	}

	// The wavefront is symmetric about the axis
	QPolygonF result;
	for (int i = upper.size() - 1; i > 0; i--)
		result << QPointF(upper[i].x(), -upper[i].y());
	result << upper;
	return result;
}

/// Draw wavefronts at regularly spaced positions along the visible part of the beam
void BeamItem::drawWavefronts(QPainter* painter, const QColor& color, double horizontalScale, double verticalScale) const
{
	const double h = fabs(horizontalScale);
	if ((h <= 0.) || (fabs(verticalScale) <= 0.))
		return;

	// Only the part of the beam that is on screen
	QRectF visible = painter->worldTransform().inverted().mapRect(
		QRectF(0., 0., painter->device()->width(), painter->device()->height()));
	const double left = qMax(m_leftBound, visible.left());
	const double right = qMin(m_rightBound, visible.right());
	if (left >= right)
		return;

	// About 40 pixels apart, rounded to 1, 2 or 5 x 10^n metres, at multiples of
	// that spacing so the wavefronts stay in place when scrolling
	const double target = 40.*h;
	const double magnitude = pow(10., floor(log10(target)));
	const double mantissa = target/magnitude;
	const double spacing = magnitude*(mantissa < 1.5 ? 1. : mantissa < 3.5 ? 2. : mantissa < 7.5 ? 5. : 10.);

	QPen pen(color);
	pen.setCosmetic(true);
	painter->save();
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);

	int count = 0;
	for (double position = ceil(left/spacing)*spacing; (position < right) && (count < 1000); position += spacing, count++)
		if ((position > m_leftBound) && (position < m_rightBound))
			painter->drawPolyline(wavefront(position, horizontalScale, verticalScale));

	painter->restore();
}

void BeamItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(widget);

	QColor beamColor = wavelengthColor(m_beam->wavelength());
	beamColor.setAlpha(200);

	// The beam is slightly transparent, so on the dark background it would look
	// darker. In dark mode, draw it opaque in the colour it has on the light
	// background (blended with white), so beams look the same in both modes.
	QColor drawColor = beamColor;
	if (Theme::instance()->isDark())
	{
		const double a = beamColor.alphaF();
		drawColor = QColor::fromRgbF(a*beamColor.redF()   + (1. - a),
		                             a*beamColor.greenF() + (1. - a),
		                             a*beamColor.blueF()  + (1. - a));
	}

	QPen beamPen(drawColor);
	beamPen.setCosmetic(true); // 1 pixel wide regardless of zoom (see OpticsItem::paint)
	painter->setPen(beamPen);

	QBrush beamBrush(drawColor, m_style ? Qt::SolidPattern : Qt::NoBrush);
	painter->setBrush(beamBrush);

	QPen textPen(Theme::viewForeground());
	textPen.setCosmetic(true);

	const double waist = m_beam->waist();
	const double waistPosition = m_beam->waistPosition();
	const double rayleigh = m_beam->rayleigh();

	// option->matrix is no longer filled in since Qt 4.6/5; use the painter's transform
	Q_UNUSED(option);
	double horizontalScale = 1./painter->worldTransform().m11(); // m/Pixels
	double verticalScale   = 1./painter->worldTransform().m22(); // m/Pixels

	// The waist is pixel reoslved
	if (waist/verticalScale > 1.)
	{
		QPolygonF beamPolygonUp, beamPolygonDown;

		// Construct the upper part of the beam
		double nextLeft, left = m_leftBound;
		drawSegment(left, nextLeft = waistPosition - 5.*rayleigh, horizontalScale, 1,  beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition - rayleigh,    horizontalScale, 10, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition + rayleigh,    horizontalScale, 20, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = waistPosition + 5.*rayleigh, horizontalScale, 10, beamPolygonUp); left = nextLeft;
		drawSegment(left, nextLeft = m_rightBound,                horizontalScale, 1,  beamPolygonUp); left = nextLeft;
		beamPolygonUp.append(QPointF(m_rightBound, m_beam->radius(m_rightBound)));

		// Mirror the upper part to make the lower part
		for (int i = beamPolygonUp.size() - 1; i >= 0; i--)
			beamPolygonDown.append(QPointF(beamPolygonUp[i].x(), -beamPolygonUp[i].y()));

		// Draw the beam
		QPainterPath path;
		path.moveTo(beamPolygonUp[0]);
		path.addPolygon(beamPolygonUp);
		path.lineTo(beamPolygonDown[0]);
		path.addPolygon(beamPolygonDown);
		path.lineTo(beamPolygonUp[0]);
		painter->drawPath(path);
	}
	// The waist is not pixel resolved
	else
	{
		double sgn = sign((m_rightBound - waistPosition)*(m_leftBound - waistPosition));
		double rightRadius = m_beam->radius(m_rightBound);
		double leftRadius = m_beam->radius(m_leftBound);

		QPolygonF ray;
		ray << QPointF(m_leftBound, leftRadius)
			<< QPointF(m_rightBound, sgn*rightRadius)
			<< QPointF(m_rightBound, -sgn*rightRadius)
			<< QPointF(m_leftBound, -leftRadius);
		painter->drawConvexPolygon(ray);
	}

	// Wavefronts, drawn on the main beams only (not on the target beam)
	OpticsScene* opticsScene = dynamic_cast<OpticsScene*>(scene());
	if (m_style && !m_auxiliary && opticsScene && opticsScene->wavefrontsVisible())
		drawWavefronts(painter, beamColor.darker(170), horizontalScale, verticalScale);

	// Waist label
	/// @todo check if in view
	if (m_drawText)
	{
		painter->setPen(textPen);
		QPointF waistTop(waistPosition, - m_beam->waist());
		painter->drawLine(QPointF(waistPosition, 0.), waistTop);
		QString text; text.setNum(round(m_beam->waist()*Units::getUnit(UnitWaist).divider()));
		// Draw the label in pixel coordinates, otherwise the 100x15 text box
		// would be 100 m x 15 m in scene coordinates
		QPointF waistTopPixel = painter->transform().map(waistTop);
		painter->save();
		painter->resetTransform();
		QRectF textRect(0., 0., 100., 15.);
		textRect.moveCenter(waistTopPixel - QPointF(0., 10.));
		painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, text);
		painter->restore();
	}
}
