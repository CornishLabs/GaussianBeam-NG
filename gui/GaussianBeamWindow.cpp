/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>
   Modified 2026 by Daniel Ruttley: ported to Qt 5 and Windows; dark mode toggle and new icons; fixed opening a file from the command line (see CHANGES.md)

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

#include "gui/GaussianBeamWindow.h"
#include "gui/OpticsView.h"
#include "gui/OpticsWidgets.h"
#include "gui/Unit.h"
#include "gui/Theme.h"

#include <QDebug>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QDockWidget>
#include <QSettings>
#include <QHeaderView>
#if QT_VERSION >= 0x050000
#include <QGuiApplication>
#include <QScreen>
#else
#include <QApplication>
#include <QDesktopWidget>
#endif

GaussianBeamWindow::GaussianBeamWindow(const QString& fileName)
	: QMainWindow()
	, OpticsBenchNotify(m_globalBench)
{
	m_currentFile = QString();

	setupUi(this);
	initSaveVariables();

	// Table
	m_tableConfigWidget = new TablePropertySelector(this);
	m_tableConfigWidget->setWindowFlags(Qt::Window);
	m_tableCornerWidget = new CornerWidget("settings", m_tableConfigWidget, this);
	m_tableCornerWidget->setToolTip(tr("Configure table columns"));
	m_model = new GaussianBeamModel(m_bench, m_tableConfigWidget, this);
	m_table = new QTableView(this);
	m_table->setModel(m_model);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//m_table->setShowGrid(false);
	m_table->verticalHeader()->hide();
	m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_table->setAlternatingRowColors(true);
	m_selectionModel = new QItemSelectionModel(m_model);
	m_table->setSelectionModel(m_selectionModel);
	m_table->resizeColumnsToContents();
	GaussianBeamDelegate* delegate = new GaussianBeamDelegate(this, m_model, m_bench);
	m_table->setItemDelegate(delegate);
	m_table->setCornerWidget(m_tableCornerWidget);
	connect(m_model, SIGNAL(modelReset()), m_table, SLOT(resizeColumnsToContents()));

	for (int i = 0; i < 2; i++)
		m_bench.addOptics(LensType, m_bench.nOptics());

	// View
	m_opticsScene = new OpticsScene(m_bench, this);
	m_opticsView = new OpticsView(m_opticsScene);
	m_opticsView->setHorizontalRange(0.60);
	m_opticsView->setVerticalRange(0.002);

	// Widget
	m_widget = new GaussianBeamWidget(m_bench, m_opticsScene, this);

	// Wavelength widget
	QWidget* wavelengthWidget = new QWidget(this);
	QVBoxLayout* wavelengthLayout = new QVBoxLayout(wavelengthWidget);
	QLabel* wavelengthLabel = new QLabel(tr("Wavelength"), wavelengthWidget);
	wavelengthSpinBox = new QDoubleSpinBox(wavelengthWidget);
	wavelengthSpinBox->setDecimals(0);
	wavelengthSpinBox->setSuffix(" nm");
	wavelengthSpinBox->setRange(1., 9999.);
	wavelengthSpinBox->setValue(532.);
	wavelengthSpinBox->setSingleStep(10.);
	wavelengthLayout->addWidget(wavelengthSpinBox);
	wavelengthLayout->addWidget(wavelengthLabel);
	wavelengthWidget->setLayout(wavelengthLayout);
	connect(wavelengthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_WavelengthSpinBox_valueChanged(double)));

	// Bars
	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_fileToolBar->addAction(action_Open);
	m_fileToolBar->addAction(action_Save);
	m_fileToolBar->addAction(action_SaveAs);
	m_fileToolBar->addSeparator();
	m_fileToolBar->addAction(action_AddOptics);
	m_fileToolBar->addAction(action_RemoveOptics);
	m_fileToolBar->addSeparator();
	m_fileToolBar->addWidget(wavelengthWidget);
	// Light/dark mode toggle, pushed to the right-hand end of the toolbar.
	// The toolbar cannot be moved, so the toggle stays in the top-right corner.
	m_fileToolBar->setMovable(false);
	QWidget* toolBarSpacer = new QWidget(m_fileToolBar);
	toolBarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_fileToolBar->addWidget(toolBarSpacer);
	m_themeAction = new QAction(this);
	m_fileToolBar->addAction(m_themeAction);
	if (QToolButton* themeButton = qobject_cast<QToolButton*>(m_fileToolBar->widgetForAction(m_themeAction)))
		themeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	connect(m_themeAction, SIGNAL(triggered()), Theme::instance(), SLOT(toggle()));
	connect(Theme::instance(), SIGNAL(changed()), this, SLOT(updateIcons()));
	updateIcons();

	StatusWidget* statusWidget = new StatusWidget(statusBar());
	statusBar()->addWidget(statusWidget, 1);
	m_opticsView->setStatusWidget(statusWidget);

	// Layouts
	QSplitter *splitter = new QSplitter(Qt::Vertical, this);
	splitter->addWidget(m_table);
	splitter->addWidget(m_opticsView);
	QList<int> sizes;
	sizes << 10 << 10;
	splitter->setSizes(sizes);
	QDockWidget* dock = new QDockWidget(this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setWidget(m_widget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	setCentralWidget(splitter);

	// Connect signal and slots
	connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
	        this, SLOT(updateWidget(const QModelIndex&, const QModelIndex&)));

	m_bench.registerNotify(this);

	setCurrentFile(QString()); // sets the window title to the program name and version
	// Read the settings first, so that the last-used wavelength does not
	// replace the wavelength of a file opened from the command line
	readSettings();

	// Open a file given on the command line once the window is shown. The file's
	// view range and origin depend on the size of the view, which is only final
	// then. Opened here, the view showed the wrong part of the bench (often empty).
	if (!fileName.isEmpty())
	{
		m_startupFile = fileName;
		QTimer::singleShot(0, this, SLOT(openStartupFile()));
	}
}

void GaussianBeamWindow::openStartupFile()
{
	openFile(m_startupFile);
}

void GaussianBeamWindow::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	writeSettings();
}

void GaussianBeamWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("GaussianBeamWindow/size", size());
	settings.setValue("GaussianBeamWindow/pos", pos());
	settings.setValue("GaussianBeamWindow/wavelength", m_bench.wavelength());
}

void GaussianBeamWindow::readSettings()
{
	QSettings settings;
	// Always open on the primary monitor, centered, ignoring the saved
	// position (which may be on a monitor that is no longer connected).
#if QT_VERSION >= 0x050000
	QRect available = QGuiApplication::primaryScreen()->availableGeometry();
#else
	QRect available = QApplication::desktop()->availableGeometry(QApplication::desktop()->primaryScreen());
#endif
	QSize windowSize = settings.value("GaussianBeamWindow/size", QSize(800, 600)).toSize();
	windowSize = windowSize.boundedTo(available.size() - QSize(40, 80)); // leave room for the title bar
	resize(windowSize);
	move(available.center() - QPoint(windowSize.width()/2, windowSize.height()/2));
	m_bench.setWavelength(settings.value("GaussianBeamWindow/wavelength", 461e-9).toDouble());
}

void GaussianBeamWindow::on_action_AddOptics_triggered()
{
	QWidget* button = m_fileToolBar->widgetForAction(action_AddOptics);

	QMenu menu(this);
	menu.addAction(action_AddLens);
	menu.addAction(action_AddFlatMirror);
	menu.addAction(action_AddCurvedMirror);
	menu.addAction(action_AddFlatInterface);
	menu.addAction(action_AddCurvedInterface);
	menu.addAction(action_AddDielectricSlab);
	menu.addAction(action_AddGenericABCD);
	menu.exec(button->mapToGlobal(QPoint(0, button->height())));
}

void GaussianBeamWindow::on_action_RemoveOptics_triggered()
{
	for (int row = m_model->rowCount() - 1; row >= 0; row--)
		if ((m_bench.optics(row)->type() != CreateBeamType) &&
		    m_selectionModel->isRowSelected(row, QModelIndex()))
			m_bench.removeOptics(row);
}

void GaussianBeamWindow::on_WavelengthSpinBox_valueChanged(double wavelength)
{
	m_bench.setWavelength(wavelength*Units::getUnit(UnitWavelength).multiplier());
}

/// Set the icons in the colours of the current theme, and the light/dark toggle
void GaussianBeamWindow::updateIcons()
{
	action_Open->setIcon(Theme::icon("open"));
	action_Save->setIcon(Theme::icon("save"));
	action_SaveAs->setIcon(Theme::icon("save-as"));
	action_Print->setIcon(Theme::icon("print"));
	action_AddOptics->setIcon(Theme::icon("add"));
	action_RemoveOptics->setIcon(Theme::icon("remove"));

	// Shows the mode that clicking switches to
	const bool dark = Theme::instance()->isDark();
	m_themeAction->setIcon(Theme::icon(dark ? "sun" : "moon"));
	m_themeAction->setText(dark ? tr("Light mode") : tr("Dark mode"));
	m_themeAction->setToolTip(dark ? tr("Switch to light mode") : tr("Switch to dark mode"));
}

void GaussianBeamWindow::OpticsBenchWavelengthChanged()
{
	wavelengthSpinBox->setValue(m_bench.wavelength()*Units::getUnit(UnitWavelength).divider());
}

void GaussianBeamWindow::insertOptics(OpticsType opticsType)
{
	QModelIndex index = m_selectionModel->currentIndex();
	int row = m_model->rowCount();
	if (index.isValid() && m_selectionModel->hasSelection())
		row = index.row() + 1;

	m_bench.addOptics(opticsType, row);
	m_table->resizeColumnsToContents();
}

void GaussianBeamWindow::openFile(const QString& path)
{
	QSettings settings;
	QString fileName = path;
	QString dir = settings.value("GaussianBeamWindow/lastDirectory", "").toString();

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this, tr("Choose a data file"), dir, "*.xml");
	if (fileName.isEmpty())
		return;

	if (parseFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("loaded"));
		settings.setValue("GaussianBeamWindow/lastDirectory", QFileInfo(fileName).path());
	}
}

void GaussianBeamWindow::saveFile(const QString& path)
{
	QSettings settings;
	QString fileName = path;
	QString dir = settings.value("GaussianBeamWindow/lastDirectory", "").toString();

	if (fileName.isNull())
		fileName = QFileDialog::getSaveFileName(this, tr("Save File"), dir, "*.xml");
	if (fileName.isEmpty())
		return;
	if (!fileName.endsWith(".xml"))
		fileName += ".xml";

	if (writeFile(fileName))
	{
		setCurrentFile(fileName);
		statusBar()->showMessage(tr("File") + " " + QFileInfo(fileName).fileName() + " " + tr("saved"));
		settings.setValue("GaussianBeamWindow/lastDirectory", QFileInfo(fileName).path());
	}
}

void GaussianBeamWindow::setCurrentFile(const QString& path)
{
	m_currentFile = path;
	if (!m_currentFile.isEmpty())
		setWindowTitle(QFileInfo(m_currentFile).fileName() + " - " APP_NAME " " APP_VERSION);
	else
		setWindowTitle(APP_NAME " " APP_VERSION);
}

void GaussianBeamWindow::updateWidget(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
	m_table->resizeRowsToContents();
}
