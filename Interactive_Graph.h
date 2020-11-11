#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>

//enum class Unit_Type
//{
//	WAVE_NUMBER = 0,
//	WAVELENGTH_MICRONS = 1,
//	ENERGY_EV = 2
//};

enum class Unit_Type
{
	CURRENT_A = 0,
	CURRENT_MA = 1,
	CURRENT_UA = 2,
	CURRENT_NA = 3
};

enum class Y_Unit_Type
{
	CURRENT_A = 0,
	CURRENT_A_PER_AREA_CM = 1,
	LOG_CURRENT_A = 2,
	LOG_CURRENT_A_PER_AREA_CM = 3,
	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4
};

using Metadata = std::vector<QVariant>;

struct Single_Graph
{
	QVector<double> x_data;
	QVector<double> y_data;
	QCPGraph* graph_pointer;
	bool allow_y_scaling;
	Metadata meta;
};

class Interactive_Graph :
	public QCustomPlot
{
	Q_OBJECT


signals:
	void Graph_Is_Selected( QCPGraph* selected_graph );

public:
	explicit Interactive_Graph( QWidget *parent = 0 );
	void Initialize_Graph();

	void selectionChanged();
	void mousePress();
	void titleDoubleClick( QMouseEvent * event );
	void axisLabelDoubleClick( QCPAxis * axis, QCPAxis::SelectablePart part, QMouseEvent * event );
	void legendDoubleClick( QCPLegend * legend, QCPAbstractLegendItem * item, QMouseEvent * event );
	void mouseWheel();
	void refitGraphs();
	void removeSelectedGraph();
	void removeAllGraphs();
	void graphContextMenuRequest( QPoint pos );
	void moveLegend();
	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );
	void saveCurrentGraph();
	const Single_Graph & Graph( QVector<double> x_data, QVector<double> y_data, QString unique_name, Metadata meta, QString graph_title = QString(), bool allow_y_scaling = true );
	void Hide_Graph( QString graph_name );

	std::tuple< QVector<double>, QVector<double> > Convert_Units( const Single_Graph & graph );
	void RegraphAll();
	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );
	Single_Graph GetSelectedGraphData() const;
	Single_Graph FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;

	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > y_axis_menu_functions;
	Unit_Type x_axis_units = Unit_Type::CURRENT_A;
	Y_Unit_Type y_axis_units = Y_Unit_Type::CURRENT_A;
	//std::function<double( double )> x_display_method{ []( double x ) { return 10000 / x; } };
	std::function<double( double, double )> y_display_method{ []( double x, double y ) { return y; } };

private:
	std::map< QString, Single_Graph > remembered_graphs;

};

//inline double Convert_Units( Unit_Type original, Unit_Type converted, double input )
//{
//	if( original == converted )
//		return input;
//	switch( original )
//	{
//		case Unit_Type::WAVE_NUMBER:
//		switch( converted )
//		{
//			case Unit_Type::WAVELENGTH_MICRONS:
//				return 10000.0 / input;
//			break;
//			case Unit_Type::ENERGY_EV:
//				return 1.23984198406e-4 * input;
//			break;
//		};
//		break;
//		case Unit_Type::WAVELENGTH_MICRONS:
//		switch( converted )
//		{
//			case Unit_Type::WAVE_NUMBER:
//			return 10000.0 / input;
//			break;
//			case Unit_Type::ENERGY_EV:
//			return 1.23984198406 / input;
//			break;
//		};
//		break;
//		case Unit_Type::ENERGY_EV:
//		switch( converted )
//		{
//			case Unit_Type::WAVE_NUMBER:
//			return input / 1.23984198406e-4;
//			break;
//			case Unit_Type::WAVELENGTH_MICRONS:
//			return 1.23984198406 / input;
//			break;
//		};
//		break;
//	};
//
//	return -INFINITY;
//}

inline double Convert_Units( Unit_Type original, Unit_Type converted, double input )
{
	//if( original == converted )
		return input;


	return -INFINITY;
}
