/***************************************************************************
    File                 : Graph.h
    Project              : SciDAVis
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Benkert
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Graph widget

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef GRAPH_H
#define GRAPH_H

#include <QList>
#include <QPointer>
#include <QPrinter>
#include <QVector>
#include <QEvent>
#include <QMap>

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>

#include "QtEnums.h"
#include "Plot.h"
#include "Table.h"
#include "AxesDialog.h"
#include "PlotToolInterface.h"
#include "core/column/Column.h"
#include "QwtSymbol.h"
#include "Qt.h"

class QwtPlotCurve;
class QwtPlotZoomer;
class QwtPieCurve;
class Table;
class Legend;
class ArrowMarker;
class ImageMarker;
class TitlePicker;
class ScalePicker;
//class CanvasPicker;
#include "CanvasPicker.h"
class ApplicationWindow;
class Matrix;
class SelectionMoveResizer;
class RangeSelectorTool;
class DataCurve;
class PlotCurve;
class QwtErrorPlotCurve;

//! Structure containing curve layout parameters
typedef struct{
  unsigned int lCol; //!< line color
  int lWidth;      //!< line width
  int lStyle;      //!< line style
  int lCapStyle=0;   //!< line CapStyle
  int lJoinStyle=0x40;  //!< line JoinStyle
  QString lCustomDash;
  int filledArea;  //!< flag: toggles area filling under curve
  unsigned int aCol; //!< curve area color
  int aStyle;      //!< area filling style
  unsigned int symCol; //!< symbol outline color
  bool symbolFill; //!< flag: toggles symbol filling
  unsigned int fillCol; //!< symbol fill color
  int penWidth;    //!< symbol outline width
  int sSize;       //!< symbol size
  int sType;       //!< symbol type (shape)
  int connectType; //!< symbol connection type
}  CurveLayout;

/**
 * \brief A 2D-plotting widget.
 *
 * Graphs are managed by a MultiLayer, where they are sometimes referred to as "graphs" and sometimes as "layers".
 * Other parts of the code also call them "plots", regardless of the fact that there's also a class Plot.
 * Within the user interface, they are quite consistently called "layers".
 *
 * Each graph owns a Plot called #d_plot, which handles parts of the curve, axis and marker management (similarly to QwtPlot),
 * as well as the pickers #d_zoomer (a QwtPlotZoomer), #titlePicker (a TitlePicker), #scalePicker (a ScalePicker) and #cp (a CanvasPicker),
 * which handle various parts of the user interaction.
 *
 * Graph contains support for various curve types (see #CurveType),
 * some of them relying on SciDAVis-specific QwtPlotCurve subclasses for parts of the functionality.
 *
 * %Note that some of Graph's methods are implemented in analysis.cpp.
 *
 * \section future_plans Future Plans
 * Merge with Plot and CanvasPicker.
 * Think about merging in TitlePicker and ScalePicker.
 * On the other hand, things like range selection, peak selection or (re)moving data points could be split out into tool classes
 * like QwtPlotZoomer or SelectionMoveResizer.
 *
 * What definitely should be split out are plot types like histograms and pie charts (TODO: which others?).
 * We need a generic framework for this in any case so that new plot types can be implemented in plugins,
 * and Graph could do with a little diet. Especially after merging in Plot and CanvasPicker.
 * [ Framework needs to support plug-ins; assigned to knut ]
 *
 * Add support for floating-point line widths of curves and axes (request 2300).
 * [ assigned to thzs ]
 */

class Graph: public QWidget
{
  Q_OBJECT

  Plot *d_plot;
  QwtPlotZoomer *d_zoomer[2];
  TitlePicker *titlePicker;
  ScalePicker *scalePicker;
  CanvasPicker* cp;

public:
  Graph (QWidget* parent=0, QString name=QString(), Qt::WindowFlags f=0);
  ~Graph();

  enum Axis{Left=QwtPlot::yLeft, Right=QwtPlot::yRight, Bottom=QwtPlot::xBottom,
            Top=QwtPlot::xTop};
  enum AxisType{Numeric = 0, Txt = 1, Day = 2, Month = 3, Time = 4, Date = 5, ColHeader = 6, DateTime = 22};
  enum MarkerType{None = -1, Text = 0, Arrow = 1, Image = 2};
  enum CurveType{Line, Scatter, LineSymbols, VerticalBars, Area, Pie, VerticalDropLines,
                 Spline, HorizontalSteps, Histogram, HorizontalBars, VectXYXY, ErrorBars,
                 Box, VectXYAM, VerticalSteps, ColorMap, GrayMap, ContourMap, Function};

  static int mapToQwtAxis(int axis);

  //! Returns the name of the parent MultiLayer object.
  QString parentPlotName();

  //! Change the active tool, deleting the old one if it exists.
  void setActiveTool(PlotToolInterface *tool);
  //! Return the active tool, or NULL if none is active.
  PlotToolInterface* activeTool() const { return d_active_tool; }

  Grid& grid(){
    if (auto g=d_plot->grid())
      return *g;
    else
      throw NoSuchObject();
  };

  void exportPainter(QPaintDevice &paintDevice, bool keepAspect = false, QRect rect = QRect());
  void exportPainter(QPainter &painter, bool keepAspect = false, QRect rect = QRect(), QSize size = QSize());
public slots:
  //! Accessor method for #d_plot.
  Plot* plotWidget() const {return d_plot;};
  void copy(ApplicationWindow *parent, Graph* g);

  //! \name Pie Curves
  //@{
  //! Returns true if this Graph is a pie plot, false otherwise.
  bool isPiePlot() const {return (c_type.count() == 1 && c_type[0] == Pie);};
  void plotPie(Table* w,const QString& name, int startRow = 0, int endRow = -1);
  //! Used when restoring a pie plot from a project file
  void plotPie(Table* w,const QString& name, const QPen& pen, int brush, int size, int firstColor, int startRow = 0, int endRow = -1, bool visible = true);
  void removePie();
  QString pieLegend();
  QString savePieCurveLayout();
  //@}

  bool insertCurvesList(Table* w, const QStringList& names, int style, int lWidth, int sSize, int startRow = 0, int endRow = -1);
  bool insertCurve(Table* w, const QString& name, int style, int startRow, int endRow);
  bool insertCurve(Table* w, const QString& name, int style, int startRow)
  {return insertCurve(w,name,style,startRow,-1);}
  bool insertCurve(Table* w, const QString& name, int style)
  {return insertCurve(w,name,style,0);}
  bool insertCurve(Table* w, int xcol, const QString& name, int style);
  bool insertCurve(Table* w, const QString& xColName, const QString& yColName, int style, int startRow, int endRow);
  bool insertCurve(Table* w, const QString& xColName, const QString& yColName, int style, int startRow)
  {return insertCurve(w,xColName,yColName,style,startRow,-1);}
  bool insertCurve(Table* w, const QString& xColName, const QString& yColName, int style)
  {return insertCurve(w,xColName,yColName,style,0);}

  bool insertPolarCurve(const QString &radial, const QString &angular,
			double from, double to, const QString &parameter, int points,
			const QString &title);
  bool insertPolarCurve(const QString &radial, const QString &angular,
			double from, double to, const QString &parameter, int points)
  {return insertPolarCurve(radial,angular,from,to,parameter,points,{});}
  bool insertPolarCurve(const QString &radial, const QString &angular,
			double from, double to, const QString &parameter)
  {return insertPolarCurve(radial,angular,from,to,parameter,100);}
  bool insertPolarCurve(const QString &radial, const QString &angular,
			double from, double to)
  {return insertPolarCurve(radial,angular,from,to,"t");}
  bool insertPolarCurve(const QString &radial, const QString &angular,
			double from)
  {return insertPolarCurve(radial,angular,from,2*M_PI);}
  bool insertPolarCurve(const QString &radial, const QString &angular)
  {return insertPolarCurve(radial,angular,0);}

  bool insertParametricCurve(const QString &x, const QString &y,
                             double from, double to, const QString &parameter, int points,
                             const QString &title);
  bool insertParametricCurve(const QString &x, const QString &y,
                             double from, double to, const QString &parameter, int points)
  {return insertParametricCurve(x,y,from,to,parameter,points,{});}
  bool insertParametricCurve(const QString &x, const QString &y,
                             double from, double to, const QString &parameter)
  {return insertParametricCurve(x,y,from,to,parameter,100);}
  bool insertParametricCurve(const QString &x, const QString &y,double from, double to)
  {return insertParametricCurve(x,y,from,to,"t");}
  bool insertParametricCurve(const QString &x, const QString &y,double from)
  {return insertParametricCurve(x,y,from,1);}
  bool insertParametricCurve(const QString &x, const QString &y)
  {return insertParametricCurve(x,y,0);}

  
  void insertPlotItem(QwtPlotItem *i, int type);

  //! Shows/Hides a curve defined by its index.
  void showCurve(int index, bool visible = true);
  int visibleCurves();

  //! Removes a curve defined by its index.
  void removeCurve(int index);
  /**
   * \brief Removes a curve defined by its title string s.
   */
  void removeCurve(const QString& s);
  /**
   * \brief Removes all curves defined by the title/plot association string s.
   */
  void removeCurves(const QString& s);

  void updateCurvesData(Table* w, const QString& yColName);

  int curves() const {return n_curves;};
  bool validCurvesDataSize() const;
  double selectedXStartValue();
  double selectedXEndValue();

  long curveKey(int curve){return c_keys[curve];}
  int curveIndex(long key) const {return c_keys.indexOf(key);}
  //! Map curve pointer to index.
  int curveIndex(QwtPlotCurve *c) const;
  //! map curve title to index
  int curveIndex(const QString &title) const {return plotItemsList().indexOf(title);}
  //! get curve by index
  QwtPlotCurve* curvePtr(int index) const;
  SciQwtPlotCurve curve(int index) const {
    if (auto r=curvePtr(index)) return *r;
    throw NoSuchObject();
  }
  //! get curve by name
  QwtPlotCurve* curvePtr(const QString &title) const {return curvePtr(curveIndex(title));}
  SciQwtPlotCurve curve(const QString &title) const {return curve(curveIndex(title));}

  //! Returns the names of all the curves suitable for data analysis, as a string list. The list excludes error bars and spectrograms.
  QStringList analysableCurvesList();
  //! Returns the names of all the QwtPlotCurve items on the plot, as a string list
  QStringList curvesList();
  //! Returns the names of all plot items, including spectrograms, as a string list
  QStringList plotItemsList() const;
  //! get plotted item by index
  QwtPlotItem* plotItem(int index);
  //! get plot item by index
  int plotItemIndex(QwtPlotItem *it) const;

  void updateCurveNames(const QString& oldName, const QString& newName, bool updateTableName = true);

  int curveType(int curveIndex);
  //! Test whether curve can be converted to type using setCurveType().
  static bool canConvertTo(QwtPlotCurve *curve, CurveType type);
  //! Change the type of the given curve.
  /**
   * The option to disable updating is provided so as not to break the project opening code
   * (ApplicationWindow::openGraph()).
   */
  void setCurveType(int curve, CurveType type, bool update=true);
  void setCurveFullRange(int curveIndex);

  //! \name Output: Copy/Export/Print
  //@{
  void print();
  void setScaleOnPrint(bool on){d_scale_on_print = on;};
  void printCropmarks(bool on){d_print_cropmarks = on;};

  void copyImage();
  //! Provided for convenience in scripts
  void exportToFile(const QString& fileName);
  void exportSVG(const QString& fname);
  void exportVector(const QString& fileName, int res = 0, bool color = true,
                    bool keepAspect = true, QPrinterEnum::PageSize pageSize = QPrinterEnum::Custom, 
                    QPrinterEnum::Orientation orientation = QPrinterEnum::Portrait);
  void exportImage(const QString& fileName, int quality);
  void exportImage(const QString& fileName) {exportImage(fileName,-1);}
  //@}

  void replot(){d_plot->replot();};
  void updatePlot();

  //! \name Error Bars
  //@{
  bool addErrorBars(const QString& xColName, const QString& yColName, Table& errTable,
                    const QString& errColName, int type=1, int width=1, int cap=8,
                    const QColor& color=Qt::black,
                    bool through=true, bool minus=true, bool plus=true);

  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width, int cap, const QColor& color, bool through, bool minus,bool plus);
  
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width, int cap, const QColor& color, bool through, bool minus)
  {return addErrorBars(yColName,errTable,errColName,type,width,cap,color,through,minus,true);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width, int cap, const QColor& color, bool through)
  {return addErrorBars(yColName,errTable,errColName,type,width,cap,color,through,true);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width, int cap, const QColor& color)
  {return addErrorBars(yColName,errTable,errColName,type,width,cap,color,true);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width, int cap)
  {return addErrorBars(yColName,errTable,errColName,type,width,cap,Qt::black);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName,
                    int type, int width)
  {return addErrorBars(yColName,errTable,errColName,type,width,8);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName, int type)
  {return addErrorBars(yColName,errTable,errColName,type,1);}
  bool addErrorBars(const QString& yColName, Table& errTable, const QString& errColName)
  {return addErrorBars(yColName,errTable,errColName,1);}

  void updateErrorBars(QwtErrorPlotCurve *er, bool xErr,int width, int cap, const QColor& c, bool plus, bool minus, bool through);

  //! Returns a valid master curve for the error bars curve.
  DataCurve* masterCurve(QwtErrorPlotCurve *er);
  //! Returns a valid master curve for a plot association.
  DataCurve* masterCurve(const QString& xColName, const QString& yColName);
  //@}

  //! \name Event Handlers
  //@{
  void contextMenuEvent(QContextMenuEvent *);
  void closeEvent(QCloseEvent *e);
  bool focusNextPrevChild ( bool next );
  //@}

  //! Set axis scale
  void setScale(int axis, double start, double end, double step,
                int majorTicks, int minorTicks, int type, bool inverted);
  void setScale(int axis, double start, double end, double step,
                int majorTicks, int minorTicks, int type)
  {setScale(axis,start,end,step,majorTicks,minorTicks,type,false);}
  void setScale(int axis, double start, double end, double step,
                int majorTicks, int minorTicks)
  {setScale(axis,start,end,step,majorTicks,minorTicks,0);}
  void setScale(int axis, double start, double end, double step, int majorTicks)
  {setScale(axis,start,end,step,majorTicks,5);}
  void setScale(int axis, double start, double end, double step)
  {setScale(axis,start,end,step,5);}
  void setScale(int axis, double start, double end)
  {setScale(axis,start,end,0);}
  double axisStep(int axis){return d_user_step[axis];};

  //! \name Curves Layout
  //@{
  CurveLayout initCurveLayout(int style, int curves = 0);
  static CurveLayout initCurveLayout();
  //! Set layout parameters of the curve given by index.
  void updateCurveLayout(int index,const CurveLayout *cL);
  //! Tries to guess not already used curve color and symbol style
  void guessUniqueCurveLayout(int& colorIndex, int& symbolIndex);
  //@}

  //! \name Zoom
  //@{
  void zoomed (const QwtDoubleRect &);
  void zoom(bool on);
  void zoomOut();
  bool zoomOn();
  //@}

  void setAutoScale();
  void updateScale();

  //! \name Saving to File
  //@{
  QString saveToString(bool saveAsTemplate = false);
  QString saveScale();
  QString saveScaleTitles();
  QString saveFonts();
  QString saveMarkers();
  QString saveCurveLayout(int index);
  QString saveAxesTitleColors();
  QString saveAxesColors();
  QString saveEnabledAxes();
  QString saveCanvas();
  QString saveTitle();
  QString saveAxesTitleAlignement();
  QString saveEnabledTickLabels();
  QString saveTicksType();
  QString saveCurves();
  QString saveLabelsFormat();
  QString saveLabelsRotation();
  QString saveAxesLabelsType();
  QString saveAxesBaseline();
  QString saveAxesFormulas();
  //@}

  //! \name Text Markers
  //@{
  void drawText(bool on);
  bool drawTextActive(){return drawTextOn;};
  long insertTextMarker(Legend* mrk);

  //! Used when opening a project file
  long insertTextMarker(const QStringList& list, int fileVersion);
  void updateTextMarker(const QString& text,int angle, int bkg,const QFont& fnt,
                        const QColor& textColor, const QColor& backgroundColor);

  QFont defaultTextMarkerFont(){return defaultMarkerFont;};
  QColor textMarkerDefaultColor(){return defaultTextMarkerColor;};
  QColor textMarkerDefaultBackground(){return defaultTextMarkerBackground;};
  int textMarkerDefaultFrame(){return defaultMarkerFrame;};
  void setTextMarkerDefaults(int f, const QFont &font, const QColor& textCol, const QColor& backgroundCol);

  void setCopiedMarkerType(Graph::MarkerType type){selectedMarkerType=type;};
  void setCopiedMarkerEnds(const QPoint& start, const QPoint& end);
  void setCopiedTextOptions(int bkg, const QString& text, const QFont& font,
                            const QColor& color, const QColor& bkgColor);
  void setCopiedArrowOptions(int width, QtPenStyle style, const QColor& color,
                             bool start, bool end, int headLength, int headAngle, bool filledHead);
  void setCopiedImageName(const QString& fn){auxMrkFileName=fn;};
  QRect copiedMarkerRect(){return QRect(auxMrkStart, auxMrkEnd);};
  QVector<int> textMarkerKeys(){return d_texts;};
  Legend* textMarker(long id);

  void addTimeStamp();

  void removeLegend();
  void removeLegendItem(int index);
  void addLegendItem(const QString& colName);
  void insertLegend(const QStringList& lst, int fileVersion);
  Legend& legend();
  Legend& newLegend();
  Legend& newLegend(const QString& text);
  bool hasLegend(){return legendMarkerID >= 0;};

  //! Creates a new legend text using the curves titles
  QString legendText();
  //@}

  //! \name Line Markers
  //@{
  ArrowMarker* arrow(long id);
  void addArrow(ArrowMarker* mrk);

  //! Used when opening a project file
  void addArrowProject(QStringList list, int fileVersion);
  QVector<int> lineMarkerKeys(){return d_lines;};

  //!Draws a line/arrow depending on the value of "arrow"
  void drawLine(bool on, bool arrow = false);
  bool drawArrow(){return drawArrowOn;};
  bool drawLineActive(){return drawLineOn;};

  QtPenStyle arrowLineDefaultStyle(){return defaultArrowLineStyle;};
  bool arrowHeadDefaultFill(){return defaultArrowHeadFill;};
  int arrowDefaultWidth(){return defaultArrowLineWidth;};
  int arrowHeadDefaultLength(){return defaultArrowHeadLength;};
  int arrowHeadDefaultAngle(){return defaultArrowHeadAngle;};
  QColor arrowDefaultColor(){return defaultArrowColor;};

  void setArrowDefaults(int lineWidth,  const QColor& c, QtPenStyle style,
                        int headLength, int headAngle, bool fillHead);
  bool arrowMarkerSelected();
  //@}

  //! \name Image Markers
  //@{
  ImageMarker* imageMarker(long id);
  QVector<int> imageMarkerKeys(){return d_images;};
  ImageMarker* addImage(ImageMarker* mrk);
  ImageMarker& addImage(const QString& fileName);

  void insertImageMarker(const QStringList& lst, int fileVersion);
  bool imageMarkerSelected();
  void updateImageMarker(int x, int y, int width, int height);
  //@}

  //! \name Common to all Markers
  //@{
  void removeMarker();
  void cutMarker();
  void copyMarker();
  void pasteMarker();
  //! Keep the markers on screen each time the scales are modified by adding/removing curves
  void updateMarkersBoundingRect();

  long selectedMarkerKey();
  /*!\brief Set the selected marker.
   * \param mrk key of the marker to be selected.
   * \param add whether the marker is to be added to an existing selection.
   * If <i>add</i> is false (the default) or there is no existing selection, a new SelectionMoveResizer is
   * created and stored in #d_markers_selector.
   */
  void setSelectedMarker(long mrk, bool add=false);
  QwtPlotMarker* selectedMarkerPtr();
  bool markerSelected();
  //! Reset any selection states on markers.
  void deselectMarker();
  MarkerType copiedMarkerType(){return selectedMarkerType;};
  //@}
    	
  //! \name Axes
  //@{
  QList<int> axesType();

  QStringList scalesTitles();
  void setXTitle(const QString& text);
  void setYTitle(const QString& text);
  void setRightTitle(const QString& text);
  void setTopTitle(const QString& text);
  void setAxisTitle(int axis, const QString& text);
  QString axisTitle(int axis) { return d_plot->axisTitle(axis).text(); }

  QFont axisTitleFont(int axis);
  void setXAxisTitleFont(const QFont &fnt);
  void setYAxisTitleFont(const QFont &fnt);
  void setRightAxisTitleFont(const QFont &fnt);
  void setTopAxisTitleFont(const QFont &fnt);
  void setAxisTitleFont(int axis,const QFont &fnt);

  void setAxisFont(int axis,const QFont &fnt);
  QFont axisFont(int axis);
  void initFonts(const QFont &scaleTitleFnt,const QFont &numbersFnt);

  QColor axisTitleColor(int axis);
  void setXAxisTitleColor(const QColor& c);
  void setYAxisTitleColor(const QColor& c);
  void setRightAxisTitleColor(const QColor& c);
  void setTopAxisTitleColor(const QColor& c);
  void setAxesTitleColor(QStringList l);

  int axisTitleAlignment (int axis);
  void setXAxisTitleAlignment(int align);
  void setYAxisTitleAlignment(int align);
  void setTopAxisTitleAlignment(int align);
  void setRightAxisTitleAlignment(int align);
  void setAxesTitlesAlignment(const QStringList& align);

  QColor axisColor(int axis);
  QStringList axesColors();
  void setAxesColors(const QStringList& colors);

  QColor axisNumbersColor(int axis);
  QStringList axesNumColors();
  void setAxesNumColors(const QStringList& colors);

  void showAxis(int axis, int type, const QString& formatInfo, Table *table, bool axisOn,
                int majTicksType, int minTicksType, bool labelsOn, const QColor& c, int format,
                int prec, int rotation, int baselineDist, const QString& formula, const QColor& labelsColor);

  void enableAxis(Axis axis, bool on = true);
  QVector<bool> enabledAxes();
  void enableAxes(QVector<bool> axesOn);
  void enableAxes(const QStringList& list);

  int labelsRotation(int axis);
  void setAxisLabelRotation(int axis, int rotation);

  QStringList enabledTickLabels();
  void setEnabledTickLabels(const QStringList& list);

  void setAxesLinewidth(int width);
  //! used when opening a project file
  void loadAxesLinewidth(int width);

  void drawAxesBackbones(bool yes);
  bool axesBackbones(){return drawAxesBackbone;};
  //! used when opening a project file
  void loadAxesOptions(const QString& s);

  QList<int> axesBaseline();
  void setAxesBaseline(const QList<int> &lst);
  void setAxesBaseline(QStringList &lst);

  void setMajorTicksType(const QList<int>& lst);
  void setMajorTicksType(const QStringList& lst);

  void setMinorTicksType(const QList<int>& lst);
  void setMinorTicksType(const QStringList& lst);

  int minorTickLength();
  int majorTickLength();
  void setAxisTicksLength(int axis, int majTicksType, int minTicksType,
                          int minLength, int majLength);
  void setTicksLength(int minLength, int majLength);
  void changeTicksLength(int minLength, int majLength);

  void setLabelsNumericFormat(const QStringList& l);
  void setLabelsNumericFormat(int axis, const QStringList& l);
  void setAxisNumericFormat(int axis, int format, int prec, const QString& formula);
  void setAxisNumericFormat(int axis, int format, int prec)
  {setAxisNumericFormat(axis,format,prec,"");}
  void setAxisNumericFormat(int axis, int format)
  {setAxisNumericFormat(axis,format,6);}
  void setLabelsDateTimeFormat(int axis, int type, const QString& formatInfo);
  void setLabelsDayFormat(int axis, int format);
  void setLabelsMonthFormat(int axis, int format);

  QString axisFormatInfo(int axis);
  QStringList axesLabelsFormatInfo(){return axesFormatInfo;};

  void setLabelsTextFormat(int axis, const Column *column, int startRow, int endRow);
  void setLabelsTextFormat(int axis, Table *table, const QString& columnName);
  void setLabelsColHeaderFormat(int axis, Table *table);

  QStringList getAxesFormulas(){return axesFormulas;};
  void setAxesFormulas(const QStringList& l){axesFormulas = l;};
  void setAxisFormula(int pos, const QString &f){axesFormulas[pos] = f;};
  //@}

  //! \name Canvas Frame
  //@{
  void drawCanvasFrame(bool frameOn, int width);
  void drawCanvasFrame(const QStringList& frame);
  void drawCanvasFrame(bool frameOn, int width, const QColor& color);
  QColor canvasFrameColor();
  int canvasFrameWidth();
  bool framed();
  //@}

  //! \name Plot Title
  //@{
  void setTitle(const QString& t);
  void setTitleFont(const QFont &fnt);
  void setTitleColor(const QColor &c);
  void setTitleAlignment(int align);

  bool titleSelected();
  void selectTitle();

  void removeTitle();
  void initTitle( bool on, const QFont& fnt);
  //@}

  //! \name Modifing insertCurve Data
  //@{
  int selectedCurveID();
  int selectedCurveIndex() { return curveIndex(selectedCurveID()); }
  QString selectedCurveTitle();
  //@}

  void disableTools();

  /*! Enables the data range selector tool.
   *
   * This one is a bit special, because other tools can depend upon an existing selection.
   * Therefore, range selection (like zooming) has to be provided in addition to the generic
   * tool interface.
   */
  bool enableRangeSelectors(const QObject *status_target=NULL, const char *status_slot="");

  //! Check wether range selectors are currently enabled.
  bool rangeSelectorsEnabled() const { return !d_range_selector.isNull(); }

  //! \name Border and Margin
  //@{
  void setMargin (int d);
  void setFrame(int width, const QColor& color);
  void setFrame(int width)
  {setFrame(width,Qt::black);}
  void setFrame()
  {setFrame(1);}
  void setBackgroundColor(const QColor& color);
  void setCanvasColor(const QColor& color);
  //@}

  void addFitCurve(QwtPlotCurve *c);
  void deleteFitCurves();
  QList<QwtPlotCurve *> fitCurvesList(){return d_fit_curves;};
  /*! Set start and end to selected X range of curve index or, if there's no selection, to the curve's total range.
   *
   * \return the number of selected or total points
   */
  int range(int index, double *start, double *end);

  //!  Used for VerticalBars, HorizontalBars and Histograms
  void setBarsGap(int curve, int gapPercent, int offset);

  //! \name Image Analysis Tools
  //@{
  void showIntensityTable();
  //@}

  //! \name User-defined Functions
  //@{
  bool modifyFunctionCurve(ApplicationWindow * parent, int curve, int type, const QStringList &formulas, const QString &var,QList<double> &ranges, int points);
  bool addFunctionCurve
  (ApplicationWindow *parent, int type, const QStringList &formulas,
   const QString& var,const QList<double> &ranges, int points,
   const QString& title = {});
  //! Used when reading from a project file.
  bool insertFunctionCurve(ApplicationWindow * parent, const QStringList& func_spec, int points, int fileVersion);
  bool insertFunctionCurve(const QString &formula, double from, double to,
                           int points, const QString &title);
  bool insertFunctionCurve
  (const QString &formula, double from, double to, int points)
  {return insertFunctionCurve(formula,from,to,points,{});}
  bool insertFunctionCurve(const QString &formula, double from, double to)
  {return insertFunctionCurve(formula,from,to,100);}
  bool insertFunctionCurve(const QString &formula, double from)
  {return insertFunctionCurve(formula,from,1);}
  bool insertFunctionCurve(const QString &formula)
  {return insertFunctionCurve(formula,0);}
  //! Returns an unique function name
  QString generateFunctionName(const QString& name = tr("F"));
  //@}

  //! Provided for convenience in scripts.
  void createTable(const QString& curveName);
  void createTable(const QwtPlotCurve* curve);
  void activateGraph();

  //! \name Vector Curves
  //@{
  void plotVectorCurve(Table* w, const QStringList& colList, int style, int startRow = 0, int endRow = -1);
  void updateVectorsLayout(int curve, const QColor& color, int width, int arrowLength, int arrowAngle, bool filled, int position,
                           const QString& xEndColName = QString(), const QString& yEndColName = QString());
  //@}

  //! \name Box Plots
  //@{
  void openBoxDiagram(Table *w, const QStringList& l, int fileVersion);
  void plotBoxDiagram(Table *w, const QStringList& names, int startRow = 0, int endRow = -1);
  //@}

  bool plotHistogram(Table *w, QStringList names, int startRow=0, int endRow=-1);

  void setCurveSymbol(int index, const SciQwtSymbol& s);
  void setCurvePen(int index, const QPen& p);
  void setCurveBrush(int index, const QBrush& b);
  void setCurveStyle(int index, int s);

  //! \name Resizing
  //@{
  bool ignoresResizeEvents(){return ignoreResize;};
  void setIgnoreResizeEvents(bool ok){ignoreResize=ok;};
  void resizeEvent(QResizeEvent *e);
  void hideEvent(QHideEvent *e);
  void scaleFonts(double factor);
  //@}

  void notifyChanges();

  void updateSecondaryAxis(int axis);
  void enableAutoscaling(bool yes){m_autoscale = yes;};
  void enableAutoscaling(){enableAutoscaling(true);};

  bool autoscaleFonts(){return autoScaleFonts;};
  void setAutoscaleFonts(bool yes){autoScaleFonts = yes;};

  static int obsoleteSymbolStyle(int type);
  static QString penStyleName(QtPenStyle style);
  static QtPenStyle getPenStyle(const QString& s);
  static QtPenStyle getPenStyle(int style);
  static Qt::BrushStyle getBrushStyle(int style);
  static void showPlotErrorMessage(QWidget *parent, const QStringList& emptyColumns);
  static QPrinterEnum::PageSize minPageSize(const QPrinter& printer, const QRect& r);

  void showTitleContextMenu();
  void copyTitle();
  void cutTitle();

  void removeAxisTitle();
  void cutAxisTitle();
  void copyAxisTitle();
  void showAxisTitleMenu(int axis);
  void showAxisContextMenu(int axis);
  void hideSelectedAxis();
  void showGrids();

  //! Convenience function enabling the grid for QwtScaleDraw::Left and Bottom Scales
  void showGrid();
  //! Convenience function enabling the grid for a user defined axis
  void showGrid(int axis);

  void showAxisDialog();
  void showScaleDialog();

  //! Add a spectrogram to the graph
  void plotSpectrogram(Matrix *m, CurveType type);
  //! Restores a spectrogram. Used when opening a project file.
  void restoreSpectrogram(ApplicationWindow *app, const QStringList& lst);

  bool antialiasing(){return d_antialiasing;};
  //! Enables/Disables antialiasing of plot items.
  void setAntialiasing(bool on = true, bool update = true);

  void deselect();
  void print(QPainter *, const QRect &rect, const QwtPlotPrintFilter &pfilter = QwtPlotPrintFilter());
signals:
  void selectedGraph (Graph*);
  void closedGraph();
  void drawTextOff();
  void drawLineEnded(bool);
  void cursorInfo(const QString&);
  void showPlotDialog(int);
  void createTable(const QString&,const QString&,QList<Column*>);

  void viewImageDialog();
  void viewTextDialog();
  void viewLineDialog();
  void viewTitleDialog();
  void modifiedGraph();
  void hiddenPlot(QWidget*);

  void showLayerButtonContextMenu();
  void showContextMenu();
  void showCurveContextMenu(int);
  void showMarkerPopupMenu();

  void showAxisDialog(int);
  void axisDblClicked(int);
  void xAxisTitleDblClicked();
  void yAxisTitleDblClicked();
  void rightAxisTitleDblClicked();
  void topAxisTitleDblClicked();

  void createIntensityTable(const QString&);
  void dataRangeChanged();
  void showFitResults(const QString&);

private:
  //! List storing pointers to the curves resulting after a fit session, in case the user wants to delete them later on.
  QList<QwtPlotCurve *>d_fit_curves;
  //! Render hint for plot items.
  bool d_antialiasing;
  bool autoScaleFonts;
  QSize hidden_size;
  bool d_scale_on_print, d_print_cropmarks;
  int selectedAxis;
  QStringList axesFormulas;
  //! Stores columns used for axes with text labels or time/date format info
  QStringList axesFormatInfo;
  QList <int> axisType;
  MarkerType selectedMarkerType;
  QwtPlotMarker::LineStyle mrklStyle;

  //! Stores the step the user specified for the four scale. If step = 0.0, the step will be calculated automatically by the Qwt scale engine.
  QVector<double> d_user_step;
  //! Curve types
  QVector<int> c_type;
  //! Curves on plot keys
  QVector<int> c_keys;
  //! Arrows/lines on plot keys
  QVector<int> d_lines;
  //! Images on plot keys
  QVector<int> d_images;
  //! Stores the identifiers (keys) of the text objects on the plot
  QVector<int> d_texts;

  QPen mrkLinePen;
  QFont auxMrkFont, defaultMarkerFont;
  QColor auxMrkColor, auxMrkBkgColor;
  QPoint auxMrkStart, auxMrkEnd;
  QtPenStyle auxMrkStyle;
  QString auxMrkFileName, auxMrkText;

  int n_curves;
  int widthLine, defaultMarkerFrame;
  QColor defaultTextMarkerColor, defaultTextMarkerBackground;
  int auxMrkAngle,auxMrkBkg,auxMrkWidth;
  int auxArrowHeadLength, auxArrowHeadAngle;
  long selectedMarker,legendMarkerID;
  bool startArrowOn, endArrowOn, drawTextOn, drawLineOn, drawArrowOn;

  bool auxFilledArrowHead, ignoreResize;
  bool drawAxesBackbone, m_autoscale;

  QColor defaultArrowColor;
  int defaultArrowLineWidth, defaultArrowHeadLength, defaultArrowHeadAngle;
  bool defaultArrowHeadFill;
  QtPenStyle defaultArrowLineStyle;

  //! The markers selected for move/resize operations or NULL if none are selected.
  QPointer<SelectionMoveResizer> d_markers_selector;
  //! The current curve selection, or NULL if none is active.
  QPointer<RangeSelectorTool> d_range_selector;
  //! The currently active tool, or NULL for default (pointer).
  PlotToolInterface *d_active_tool;
};
#endif // GRAPH_H
