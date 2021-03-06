﻿#include "Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

#include "fn.hpp"
namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

static QString Unit_Names[] = { "Current (A)",
							"Current (mA)",
							"Current (" + QString( QChar( 0x03BC ) ) + "A)",
							"Current (nA)" };

static QString Y_Unit_Names[] = { "Current (A)",
							QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
							QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
							QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),
							QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

Interactive_Graph::Interactive_Graph( QWidget *parent ) :
	QCustomPlot( parent )
{
	Initialize_Graph();
}

void Interactive_Graph::Initialize_Graph()
{
	this->setOpenGl( true );
	this->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
									QCP::iSelectLegend | QCP::iSelectPlottables );
	//this->xAxis->setRange( -5.0, 5.0 );
	//this->yAxis->setRange( 0, 1 );
	this->axisRect()->setupFullAxesBox();

	this->plotLayout()->insertRow( 0 );
	//QCPTextElement *title = new QCPTextElement( this, "Current", QFont( "sans", 17, QFont::Bold ) );
	//QCPTextElement *title = new QCPTextElement( this, "Capacitance Over Area", QFont( "sans", 17, QFont::Bold ) );
	QCPTextElement *title = new QCPTextElement( this, "I-V", QFont( "sans", 17, QFont::Bold ) );
	this->plotLayout()->addElement( 0, 0, title );

	this->yAxis->setLabel( Y_Unit_Names[ 0 ] );
	//this->yAxis->setLabel( "Capacitance (F/cm" + QString( QChar( 0x00B2 ) ) + ")" );
	//this->yAxis->setLabel( "Current (A)" );
	this->xAxis->setLabel( "Voltage (V)" );
	this->legend->setVisible( true );
	QFont legendFont = font();
	legendFont.setPointSize( 10 );
	this->legend->setFont( legendFont );
	this->legend->setSelectedFont( legendFont );
	this->legend->setSelectableParts( QCPLegend::spItems ); // legend box shall not be selectable, only legend items

																	 // connect slot that ties some axis selections together (especially opposite axes):
	connect( this, &QCustomPlot::selectionChangedByUser, this, &Interactive_Graph::selectionChanged );
	// connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
	connect( this, &QCustomPlot::mousePress, this, &Interactive_Graph::mousePress );
	connect( this, &QCustomPlot::mouseWheel, this, &Interactive_Graph::mouseWheel );

	connect( this, &QCustomPlot::mouseDoubleClick, this, &Interactive_Graph::refitGraphs );

	// make bottom and left axes transfer their ranges to top and right axes:
	connect( this->xAxis, SIGNAL( rangeChanged( QCPRange ) ), this->xAxis2, SLOT( setRange( QCPRange ) ) );
	connect( this->yAxis, SIGNAL( rangeChanged( QCPRange ) ), this->yAxis2, SLOT( setRange( QCPRange ) ) );

	// connect some interaction slots:
	connect( this, &QCustomPlot::axisDoubleClick, this, &Interactive_Graph::axisLabelDoubleClick );
	connect( this, &QCustomPlot::legendDoubleClick, this, &Interactive_Graph::legendDoubleClick );
	connect( title, &QCPTextElement::doubleClicked, this, &Interactive_Graph::titleDoubleClick );

	//// connect slot that shows a message in the status bar when a graph is clicked:
	//connect( this, SIGNAL( plottableClick( QCPAbstractPlottable*, int, QMouseEvent* ) ), this, SLOT( graphClicked( QCPAbstractPlottable*, int ) ) );

	// setup policy and connect slot for context menu popup:
	this->setContextMenuPolicy( Qt::CustomContextMenu );
	//connect( this, &QWidget::customContextMenuRequested, this, &Interactive_Graph::graphContextMenuRequest );

	this->y_axis_menu_functions.emplace_back( [this]( Interactive_Graph* graph, QMenu* menu, QPoint pos )
	{
		if( graph->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			//CURRENT_A = 0,
			//	CURRENT_A_PER_AREA_CM = 1,
			//	LOG_CURRENT_A = 2,
			//	LOG_CURRENT_A_PER_AREA_CM = 3,
			//	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4

			//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
			for( size_t index = 0; index < sizeof( Y_Unit_Names ) / sizeof( Y_Unit_Names[0] ); index++ )
			{
				QString axis_name = Y_Unit_Names[ index ];
				menu->addAction( "Change to " + axis_name, [graph, index, axis_name]
				{
					graph->y_axis_units = Y_Unit_Type( index );
					graph->yAxis->setLabel( axis_name );
					graph->RegraphAll();
				} );
			}
		}
	} );
}

void Interactive_Graph::titleDoubleClick( QMouseEvent* event )
{
	Q_UNUSED( event )
		if( QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()) )
		{
			event->accept();
			// Set the plot title by double clicking on it
			bool ok;
			QString newTitle = QInputDialog::getText( this, "Change Title", "New plot title:", QLineEdit::Normal, title->text(), &ok );
			if( ok )
			{
				title->setText( newTitle );
				this->replot();
			}
		}
}

void Interactive_Graph::axisLabelDoubleClick( QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent * event )
{
	// Set an axis label by double clicking on it
	if( part == QCPAxis::spAxisLabel ) // only react when the actual axis label is clicked, not tick label or axis backbone
	{
		event->accept();
		bool ok;
		QString newLabel = QInputDialog::getText( this, "Change Axis Label", "New axis label:", QLineEdit::Normal, axis->label(), &ok );
		if( ok )
		{
			axis->setLabel( newLabel );
			this->replot();
		}
	}
}

void Interactive_Graph::legendDoubleClick( QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent * event )
{
	// Rename a graph by double clicking on its legend item
	Q_UNUSED( legend )
		if( item ) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
		{
			event->accept();
			QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
			bool ok;
			QString newName = QInputDialog::getText( this, "Change Graph Name", "New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok );
			if( ok )
			{
				plItem->plottable()->setName( newName );
				this->replot();
			}
		}
}

void Interactive_Graph::selectionChanged()
{
	/*
	normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
	the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
	and the axis base line together. However, the axis label shall be selectable individually.

	The selection state of the left and right axes shall be synchronized as well as the state of the
	bottom and top axes.

	Further, we want to synchronize the selection of the graphs with the selection state of the respective
	legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
	or on its legend item.
	*/

	// make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) || this->xAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		this->xAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || this->xAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		this->xAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		this->xAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}
	// make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) || this->yAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		this->yAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || this->yAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		this->yAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		this->yAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}

	// synchronize selection of graphs with selection of corresponding legend items:
	for( int i = 0; i < this->graphCount(); ++i )
	{
		QCPGraph *graph = this->graph( i );
		QCPPlottableLegendItem *item = this->legend->itemWithPlottable( graph );
		if( item != nullptr && (item->selected() || graph->selected()) )
		{
			item->setSelected( true );
			graph->setSelection( QCPDataSelection( graph->data()->dataRange() ) );
			emit Graph_Is_Selected( graph );
		}
	}
}

void Interactive_Graph::mousePress()
{
	// if an axis is selected, only allow the direction of that axis to be dragged
	// if no axis is selected, both directions may be dragged

	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeDrag( this->xAxis->orientation() );
	else if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeDrag( this->yAxis->orientation() );
	else
		this->axisRect()->setRangeDrag( Qt::Horizontal | Qt::Vertical );
}

void Interactive_Graph::mouseWheel()
{
	// if an axis is selected, only allow the direction of that axis to be zoomed
	// if no axis is selected, both directions may be zoomed

	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeZoom( this->xAxis->orientation() );
	else if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeZoom( this->yAxis->orientation() );
	else
		this->axisRect()->setRangeZoom( Qt::Horizontal | Qt::Vertical );
}

void Interactive_Graph::refitGraphs()
{
	this->rescaleAxes( true );
	//this->yAxis->setRangeLower( 0 );
	//this->yAxis->setRange( -10, 110 );
	this->replot();
}

void Interactive_Graph::removeSelectedGraph()
{
	if( this->selectedGraphs().size() == 0 )
		return;

	QCPGraph* selected_graph = this->selectedGraphs().first();
	for( std::map< QString, Single_Graph >::iterator element = remembered_graphs.begin();
			element != remembered_graphs.end(); ++element )
	{
		if( element->second.graph_pointer == selected_graph )
		{
			remembered_graphs.erase( element );
			break;
		}
	}
	this->removeGraph( selected_graph );
	this->replot();
}

void Interactive_Graph::removeAllGraphs()
{
	this->clearGraphs();
	this->remembered_graphs.clear();
	this->replot();
}

void Interactive_Graph::graphContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );

	if( this->legend->visible() && this->legend->selectTest( pos, false ) >= 0 ) // context menu on legend requested
	{
		menu->addAction( "Move to top left", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignLeft) );
		menu->addAction( "Move to top center", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignHCenter) );
		menu->addAction( "Move to top right", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignRight) );
		menu->addAction( "Move to bottom right", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignRight) );
		menu->addAction( "Move to bottom left", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignLeft) );
		menu->addAction( "Turn Legend Off", [this]
		{
			this->legend->setVisible( false );
			this->replot();
		} );
	}
	else if( this->xAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
	{
		////menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
		//menu->addAction( "Change to Wavelength", [this]
		//{
		//	x_display_method = []( double x ) { return 10000.0 / x; };
		//	this->xAxis->setLabel( "Wavelength (" + QString( QChar( 0x03BC ) ) + "m)" );
		//	this->Regraph_All_Plots();
		//} );
		//menu->addAction( "Change to Wave Number", [this]
		//{
		//	x_display_method = []( double x ) { return x; };
		//	this->xAxis->setLabel( "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")" );
		//	this->Regraph_All_Plots();
		//} );
		//menu->addAction( "Change to Energy", [this]
		//{
		//	x_display_method = []( double x ) { return x * 1.239842e-4; };
		//	this->xAxis->setLabel( "Photon Energy (eV)" );
		//	this->Regraph_All_Plots();
		//} );
	}
	else if( this->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
	{
		//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
		//menu->addAction( "Change to Wavelength", [this]
		//{
		//	x_display_method = []( double x ) { return 10000.0 / x; };
		//	this->xAxis->setLabel( "Wavelength (" + QString( QChar( 0x03BC ) ) + "m)" );
		//	this->Regraph_All_Plots();
		//} );
	}
	else  // general context menu on graphs requested
	{
		//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
		if( !this->legend->visible() )
			menu->addAction( "Turn Legend On", [this]
		{
			this->legend->setVisible( true );
			this->replot();
		} );
		if( this->selectedGraphs().size() > 0 )
			menu->addAction( "Remove selected graph", this, &Interactive_Graph::removeSelectedGraph );
		if( this->graphCount() > 0 )
			menu->addAction( "Remove all graphs", this, &Interactive_Graph::removeAllGraphs );
		menu->addAction( "Save current graph", this, &Interactive_Graph::saveCurrentGraph );
	}

	for( auto menu_function : this->y_axis_menu_functions )
		menu_function( this, menu, pos );

	menu->popup( this->mapToGlobal( pos ) );
}

inline double Divide_Data_Sets( double x, double y, const QVector<double> & x_data, const QVector<double> & y_data )
{
	//// First find location in data
	//int nearest_power_of_2 = 2;
	//int size = x_data.size();
	//while( size >>= 1 ) nearest_power_of_2 <<= 1;

	//int current_guess = nearest_power_of_2 / 2;
	//int search_range = nearest_power_of_2 / 2;
	//while( search_range > 0 )
	//{
	//	search_range /= 2;
	//	if( x > x_data[ current_guess ] )
	//		current_guess = std::min( current_guess + search_range, x_data.size() - 1 );
	//	else if( x < x_data[ current_guess ] )
	//		current_guess = std::max( current_guess - search_range, 0 );
	//}

	int i = std::distance( x_data.begin(), std::lower_bound( x_data.begin(), x_data.end(), x ) );

	return (y / y_data[ i ]);
}

void Interactive_Graph::moveLegend()
{
	if( QAction* contextAction = qobject_cast<QAction*>(sender()) ) // make sure this slot is really called by a context menu action, so it carries the data we need
	{
		bool ok;
		int dataInt = contextAction->data().toInt( &ok );
		if( ok )
		{
			this->axisRect()->insetLayout()->setInsetAlignment( 0, (Qt::Alignment)dataInt );
			this->replot();
		}
	}
}

void Interactive_Graph::graphClicked( QCPAbstractPlottable *plottable, int dataIndex )
{
	//// since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
	//// usually it's better to first check whether interface1D() returns non-zero, and only then use it.
	//double dataValue = plottable->interface1D()->dataMainValue( dataIndex );
	//QString message = QString( "Clicked on graph '%1' at data point #%2 with value %3." ).arg( plottable->name() ).arg( dataIndex ).arg( dataValue );
	//ui.statusBar->showMessage( message, 2500 );
}

const Single_Graph & Interactive_Graph::Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, Metadata meta, QString graph_title, bool allow_y_scaling )
{
	if( x_data.size() != y_data.size() )
		throw "Trying to graph different x and y size arrays";

	auto existing_graph = remembered_graphs.find( measurement_name );
	if( existing_graph != remembered_graphs.end() )
	{
		Single_Graph & current_info = existing_graph->second;
		if( !meta.empty() )
			current_info.meta = meta;

		if( current_info.x_data.size() == x_data.size() &&
			current_info.y_data.size() == y_data.size() )
		{
			current_info.x_data = x_data;
			current_info.y_data = y_data;
			auto[ plot_ready_x, plot_ready_y ] = this->Convert_Units( current_info );
			current_info.graph_pointer->setData( plot_ready_x, plot_ready_y );
			current_info.graph_pointer->setVisible( true );
			current_info.graph_pointer->addToLegend();

		}

		return current_info;
	}

	// Add graph
	{
		//double previous_upper_limit = this->yAxis->range().upper;
		static int color_index = 0;
		bool this_is_the_first_graph = this->graphCount() == 0;
		QCPGraph* current_graph = this->addGraph();
		if( graph_title.isEmpty() )
			current_graph->setName( measurement_name );
		else
			current_graph->setName( graph_title );

		//this->graph()->setName( meta_data.graph_title );
		// Remember data before changing it at all

		remembered_graphs[ measurement_name ] = Single_Graph{ x_data, y_data, current_graph, allow_y_scaling, meta };
		auto[ plot_ready_x, plot_ready_y ] = this->Convert_Units( remembered_graphs[ measurement_name ] );
		this->graph()->setData( plot_ready_x, plot_ready_y );
		this->graph()->setLineStyle( QCPGraph::lsLine );// (QCPGraph::LineStyle)(rand() % 5 + 1) );
																 //if( rand() % 100 > 50 )
																 //	this->graph()->setScatterStyle( QCPScatterStyle( (QCPScatterStyle::ScatterShape)(rand() % 14 + 1) ) );
		QPen graphPen;
		QCPColorGradient gradient( QCPColorGradient::gpSpectrum );// gpPolar );
		gradient.setPeriodic( true );
		//gradient.setLevelCount( 10 );
		graphPen.setColor( gradient.color( (color_index++) / 5.0, QCPRange( 0.0, 1.0 ) ) );
		//graphPen.setColor( QColor::fromHslF( (color_index++)/10.0*0.8, 0.95, 0.5) );
		//graphPen.setColor( QColor::fromHsv( rand() % 255, 255, 255 ) );
		//graphPen.setWidthF( rand() / (double)RAND_MAX * 2 + 1 );
		this->graph()->setPen( graphPen );
		//if( this_is_the_first_graph )
		{
			//this->rescaleAxes();
			//this->yAxis->setRangeLower( 0 );
			//double upper = this->yAxis->range().upper;
			////this->yAxis->setRangeUpper( std::max( previous_upper_limit, std::min( upper, 1. ) ) );
			//if( allow_y_scaling )
			//	this->yAxis->setRangeUpper( std::min( upper, 1. ) );
			//else
			//	this->yAxis->setRangeUpper( upper );
		}
		//this->yAxis->setRange( -10, 110 );

		return remembered_graphs[ measurement_name ];
	}
}

void Interactive_Graph::Hide_Graph( QString graph_name )
{
	auto existing_graph = remembered_graphs.find( graph_name );
	if( existing_graph == remembered_graphs.end() )
		return;

	Single_Graph & current_info = existing_graph->second;
	if( !current_info.graph_pointer->visible() ) // Already invisible, nothing to do
		return;
	current_info.graph_pointer->setVisible( false );
	current_info.graph_pointer->removeFromLegend();
	replot();
}

void Interactive_Graph::RegraphAll()
{
	//for( int i = 0; i < ui.customPlot->graphCount(); i++ )
	for( const auto &[ name, graph ] : remembered_graphs )
	{
		//QCPGraph* graph = ui.customPlot->graph( i );
		//QString measurement_id = graph->name();

		//QVector<double> x_data, y_data;
		//Grab_SQL_Data_From_Measurement_ID( measurement_id, x_data, y_data );

		auto[ x_data, y_data ] = Convert_Units( graph );
		if( x_data.size() != y_data.size() )
			throw "Trying to graph different x and y size arrays";

		this->UpdateGraph( graph.graph_pointer, x_data, y_data );
	}
}

extern int column_for_device_side_length;

std::tuple< QVector<double>, QVector<double> > Interactive_Graph::Convert_Units( const Single_Graph & graph )
{
	arma::vec x_data{ &graph.x_data[ 0 ], size_t(graph.x_data.size()) };
	arma::vec y_data{ &graph.y_data[ 0 ], size_t(graph.y_data.size()) };

	switch( this->y_axis_units )
	{
		case Y_Unit_Type::CURRENT_A:
		break;
		case Y_Unit_Type::CURRENT_A_PER_AREA_CM:
		{
			double side_cm = graph.meta[ column_for_device_side_length ].toDouble() * 1E-4;
			y_data /= side_cm * side_cm;
		}
		break;
		case Y_Unit_Type::LOG_CURRENT_A:
		{
			y_data = arma::log10( arma::abs( y_data ) );
		}
		break;
		case Y_Unit_Type::LOG_CURRENT_A_PER_AREA_CM:
		{
			double side_cm = graph.meta[ column_for_device_side_length ].toDouble() * 1E-4;
			y_data = arma::log10( arma::abs( y_data / (side_cm * side_cm) ) );
		}
		break;
		case Y_Unit_Type::ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM:
		{
			double side_cm = graph.meta[ column_for_device_side_length ].toDouble() * 1E-4;
			y_data = arma::log10( arma::abs( y_data / (side_cm * side_cm) ) );
			x_data = arma::abs( x_data );
		}
		break;
	}

	return { toQVec( x_data ), toQVec( y_data ) };
}

//void Interactive_Graph::RegraphAll()
//{
//	for( int i = 0; i < this->graphCount(); i++ )
//	{
//		QCPGraph* active_graph = this->graph( i );
//	}
//}
//
void Interactive_Graph::saveCurrentGraph()
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this,
														tr( "Save Graph" ), QString(), tr( "PNG File (*.png);; JPG File (*.jpg);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	if( file_name.suffix().toLower() == "png" )
		this->savePng( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "jpg" )
		this->saveJpg( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "bmp" )
		this->saveBmp( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "pdf" )
		this->savePdf( file_name.absoluteFilePath() );
}


void Interactive_Graph::UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data )
{
	//double previous_upper_limit = this->yAxis->range().upper;
	existing_graph->setData( x_data, y_data );
	//this->rescaleAxes();
	//this->yAxis->setRangeLower( 0 );
	//double upper = this->yAxis->range().upper;
	//this->yAxis->setRangeUpper( std::max( previous_upper_limit, std::min( upper, 1. ) ) );

	//this->yAxis->setRange( -10, 110 );
	this->replot();
}

Single_Graph Interactive_Graph::GetSelectedGraphData() const
{
	QList<QCPGraph*> selection = this->selectedGraphs();
	if( selection.size() == 0 )
		return {};
	else
		return FindDataFromGraphPointer( selection[ 0 ] );
}

Single_Graph Interactive_Graph::FindDataFromGraphPointer( QCPGraph* graph_pointer ) const
{
	auto result = std::find_if(
		this->remembered_graphs.begin(),
		this->remembered_graphs.end(),
		[graph_pointer]( const auto& mo ) { return mo.second.graph_pointer == graph_pointer; } );

	//RETURN VARIABLE IF FOUND
	if( result != this->remembered_graphs.end() )
		return result->second;
	else
		return {};
}
