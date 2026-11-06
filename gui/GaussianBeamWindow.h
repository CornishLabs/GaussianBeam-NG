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

#ifndef GAUSSIANBEAMWINDOW_H
#define GAUSSIANBEAMWINDOW_H

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamModel.h"
#include "gui/GaussianBeamDelegate.h"
#include "gui/OpticsView.h"
#include "ui_GaussianBeamWindow.h"
#include "src/Optics.h"
#include "src/OpticsBench.h"
#include "src/GaussianFit.h"

#include <QMainWindow>

class QXmlStreamWriter;
class CornerWidget;
class TablePropertySelector;

class GaussianBeamWindow : public QMainWindow, private OpticsBenchNotify, private Ui::GaussianBeamWindow
{
	Q_OBJECT

public:
	GaussianBeamWindow(const QString& fileName);

public slots:
	void updateWidget(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected slots:
	void on_action_Open_triggered()               { openFile();                        }
	void on_action_Save_triggered()               { saveFile(m_currentFile);           }
	void on_action_SaveAs_triggered()             { saveFile();                        }
	void on_action_AddOptics_triggered();
	void on_action_RemoveOptics_triggered();
	void on_action_AddLens_triggered()            { insertOptics(LensType);            }
	void on_action_AddFlatMirror_triggered()      { insertOptics(FlatMirrorType);      }
	void on_action_AddCurvedMirror_triggered()    { insertOptics(CurvedMirrorType);    }
	void on_action_AddFlatInterface_triggered()   { insertOptics(FlatInterfaceType);   }
	void on_action_AddCurvedInterface_triggered() { insertOptics(CurvedInterfaceType); }
	void on_action_AddGenericABCD_triggered()     { insertOptics(GenericABCDType);     }
	void on_action_AddDielectricSlab_triggered()  { insertOptics(DielectricSlabType);  }
	void on_WavelengthSpinBox_valueChanged(double wavelength);
	void updateIcons();
	void openStartupFile();

protected:
	virtual void closeEvent(QCloseEvent* event);

// optics bench inherited virtual functions
private:
	virtual void OpticsBenchWavelengthChanged();

private:
	void openFile(const QString& path = QString());
	void saveFile(const QString& path = QString());
	void setCurrentFile(const QString& path);
	void insertOptics(OpticsType opticsType);
	void readSettings();
	void writeSettings();

// Loading stuff that should logically be moved to OpticsBench, but depend on Qt.
// In addition, a GaussianBeam file contains view properties that do not belong to OpticsBench.
private:
	void initSaveVariables();
	bool parseFile(const QString& path = QString());
	void parseXml(const QDomElement& element);
	void parseBench(const QDomElement& element);
	void parseTargetBeam(const QDomElement& element);
	void parseFit(const QDomElement& element);
	void parseOptics(const QDomElement& element, QList<QString>& lockTree);
	void parseView(const QDomElement& element);
	bool writeFile(const QString& path = QString());
	void writeBench(QXmlStreamWriter& xmlWriter);
	void writeOptics(QXmlStreamWriter& xmlWriter, const Optics* optics);
	void writeView(QXmlStreamWriter& xmlWriter);
	// Compatibility functions
	void parseXml10(const QDomElement& element);

private:
	QToolBar* m_fileToolBar;
	QAction* m_themeAction;

	OpticsBench m_globalBench;
	QDoubleSpinBox* wavelengthSpinBox;
	GaussianBeamWidget* m_widget;
	GaussianBeamModel* m_model;
	QItemSelectionModel* m_selectionModel;
	QTableView* m_table;
	TablePropertySelector* m_tableConfigWidget;
	CornerWidget* m_tableCornerWidget;
	OpticsView* m_opticsView;
	OpticsScene* m_opticsScene;
	QMap<OpticsType, QString> m_opticsElements;

	QString m_currentFile;
	QString m_startupFile; ///< file given on the command line, opened once the window is shown
};

#endif
