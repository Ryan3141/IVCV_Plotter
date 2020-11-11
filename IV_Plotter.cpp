#include "IV_Plotter.h"
#include "CV_Theoretical.h"

using namespace std;
void test();

const QStringList header_titles{ "Sample Name", "Location", "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)", "Temperature (K)", "Date", "Time of Day", "measurement_id" };
const QStringList what_to_collect{ "sample_name", "device_location", "device_area_in_um2", "temperature_in_k", "date(time)", "time(time)", "measurement_id" };
const QString sql_table = "iv_measurements";
const QStringList raw_data_columns{ "measurement_id","voltage_v","current_a" };
const QString raw_data_table = "iv_raw_data";
const QString sorting_strategy = "ORDER BY measurement_id, voltage_v ASC";
const int columns_to_show = 6;
int column_for_device_side_length = what_to_collect.indexOf( "device_area_in_um2" );
const int measurement_id_column = what_to_collect.indexOf( "measurement_id" );
//const QStringList header_titles{ "Sample Name", "Date", "Temperature (K)", "Location", "Device Area", "Time of Day", "Sweep Direction", "measurement_id" };
//const QStringList what_to_collect{ "sample_name", "date(time)", "temperature_in_k", "device_location", "device_area_in_um2", "time(time)", "sweep_direction", "measurement_id" };
//const QString sql_table = "cv_measurements";
//const QStringList raw_data_columns{ "measurement_id","voltage_v","capacitance_f" };
//const QString raw_data_table = "cv_raw_data";
//const int columns_to_show = 6;

double Rule_07( double temperature, double cutoff_wavelength )
{
	double J_0 = 8367.000019;
	double Pwr = 0.544071282;
	double C = -1.162972237;
	double lambda_scale = 0.200847413;
	double lambda_threshold = 4.635136423;
	double k_B = 1.3806503e-23;
	double ee = 1.60217646e-19;

	double lambda_e = (cutoff_wavelength >= lambda_threshold) ? cutoff_wavelength :
		cutoff_wavelength / (1 - std::pow(lambda_scale / cutoff_wavelength - lambda_scale / lambda_threshold, Pwr) );

	return J_0 * std::exp( C * 1.24 * ee / (k_B * lambda_e * temperature) );
}

QString Info_Or_Default( const Metadata & meta, QString column, QString Default )
{
	if( meta.empty() )
		return Default;
	int index = header_titles.indexOf( column );
	if( index == -1 )
		return Default;
	QVariant stuff = meta[ index ];
	if( stuff == QVariant::Invalid )
		return Default;
	return meta[ index ].toString();
}

IV_Plotter::IV_Plotter(QWidget *parent)
	: QMainWindow(parent)
{
	//test();
	this->ui.setupUi(this);
	QString config_filename = "configuration.ini";
	QSettings settings( config_filename, QSettings::IniFormat, this );
	ui.sqlUser_lineEdit->setText( settings.value( "SQL_Server/default_user" ).toString() );

	sql_manager = new SQL_Manager( this );
	Initialize_Tree_Table(); // sql_manager must be initialized first
	Initialize_Graph();
	Update_Preview_Graph();
	ui.customPlot->refitGraphs();

	Add_Mouse_Position_Label();

	auto test = u8"This is a Unicode Character: μ\u2018.";
	std::cerr << test;
	//const QString pageSource = R"(
	//	<html><head>
	//	<script type = "text/javascript" src = "https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/MathJax.js?config=TeX-AMS-MML_HTMLorMML">
	//	</script></head>
	//	<body>
	//	<p><mathjax style = "font-size:2.3em">$$u = \int_{ -\infty }^{\infty}(awesome)\cdot du$$</mathjax></p>
	//	</body></html>
	//)";
	//ui.webEngineView->setHtml( pageSource );
	//ui.webEngineView->show();
}

void IV_Plotter::Initialize_Tree_Table()
{
	ui.treeWidget->Set_Data_To_Gather( header_titles, what_to_collect, columns_to_show );

	QString user = ui.sqlUser_lineEdit->text();
	ui.treeWidget->Repoll_SQL( this->sql_manager->sql_db, sql_table, user );

	QString filter = ui.filter_lineEdit->text();
	ui.treeWidget->Refilter( filter );

	connect( ui.treeWidget, &SQL_Tree_Widget::itemDoubleClicked, [this]( QTreeWidgetItem* tree_item, int column )
	{
		vector<const QTreeWidgetItem*> things_to_graph = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

		for( const QTreeWidgetItem* x : things_to_graph )
		{
			Graph_Tree_Node( x );
		}
	} );

	// setup policy and connect slot for context menu popup:
	ui.treeWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( ui.treeWidget, &QWidget::customContextMenuRequested, this, &IV_Plotter::treeContextMenuRequest );

	connect( ui.refresh_commandLinkButton, &QCommandLinkButton::clicked,
			 [this]
	{
		ui.treeWidget->Repoll_SQL( this->sql_manager->sql_db, sql_table, this->ui.sqlUser_lineEdit->text() );
		ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() );
	} );
	connect( ui.sqlUser_lineEdit, &QLineEdit::returnPressed,
			 [this]
	{
		ui.treeWidget->Repoll_SQL( this->sql_manager->sql_db, sql_table, this->ui.sqlUser_lineEdit->text() );
		ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() );
	} );
	connect( ui.filter_lineEdit, &QLineEdit::textEdited, ui.treeWidget, [this] { ui.treeWidget->Refilter( this->ui.filter_lineEdit->text() ); } );
}

void IV_Plotter::Graph_Tree_Node( const QTreeWidgetItem* tree_item )
{
	QString measurement_id = tree_item->text( measurement_id_column );
	Metadata row = ui.treeWidget->Get_Metadata_For_Row( tree_item );

	XY_Data data = sql_manager->Grab_SQL_Data_From_Measurement_IDs( raw_data_columns, raw_data_table, { measurement_id }, sorting_strategy )[ measurement_id ];

	auto & [ x, y ] = data;
	this->Graph( measurement_id, x, y, row,
					 //QString("%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3").arg( row[0].toString(), row[2].toString(), row[3].toString(), QString::number(int(std::sqrt(row[4].toInt()))) ) );
					 QString( "%1 %2 K %4" + QString( QChar( 0x03BC ) ) + "m: %3" ).arg( row[ 0 ].toString(), row[ 3 ].toString(), row[ 1 ].toString(), row[ 2 ].toString() ) );
}

void IV_Plotter::Initialize_Graph()
{
	connect( ui.customPlot, &QWidget::customContextMenuRequested, ui.customPlot, &Interactive_Graph::graphContextMenuRequest );
	connect( ui.customPlot, &Interactive_Graph::Graph_Is_Selected, [this]( QCPGraph* selected_graph )
	{
		const Single_Graph & measurement = ui.customPlot->FindDataFromGraphPointer( selected_graph );
		this->ui.selectedName_lineEdit->setText(        Info_Or_Default( measurement.meta, "Sample Name", "" ) );
		this->ui.selectedTemperature_lineEdit->setText( Info_Or_Default( measurement.meta, "Temperature (K)", "" ) );
	} );
	connect( ui.semiconductorType_comboBox, &QComboBox::currentTextChanged, [this]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.alloyComposition_doubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.semiconductorDoping_lineEdit, &QLineEdit::editingFinished, [this]() { this->Update_Preview_Graph(); } );
	connect( ui.insulatorType_comboBox, &QComboBox::currentTextChanged, [this]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.insulatorThickness_doubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.interfaceCharge_lineEdit, &QLineEdit::editingFinished, [this]() { this->Update_Preview_Graph(); } );
	connect( ui.simulatedTemperature_doubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this]( double ) { this->Update_Preview_Graph(); } );
	connect( ui.metalType_comboBox, &QComboBox::currentTextChanged, [this]( const QString & ) { this->Update_Preview_Graph(); } );
	connect( ui.displayHFPreview_checkBox, &QCheckBox::stateChanged, [this]( int ) { this->Update_Preview_Graph(); } );
	connect( ui.displayLFPreview_checkBox, &QCheckBox::stateChanged, [this]( int ) { this->Update_Preview_Graph(); } );
	connect( ui.addGraph_pushButton, &QPushButton::pressed, [this]
	{
		this->simulated_graph_number++;
		this->Update_Preview_Graph();
	} );

	connect( ui.customPlot->xAxis, qOverload<const QCPRange &>( &QCPAxis::rangeChanged ), [this]( const QCPRange & ) { this->Update_Preview_Graph(); } );
}

void IV_Plotter::Update_Preview_Graph()
{
	using namespace CV_Measurements;
	if( !ui.displayHFPreview_checkBox->isChecked() )
	{
		ui.customPlot->Hide_Graph( "HF Simulation" + QString::number( this->simulated_graph_number ) );
		//ui.customPlot->replot();
		//return;
	}

	if( !ui.displayLFPreview_checkBox->isChecked() )
	{
		ui.customPlot->Hide_Graph( "LF Simulation" + QString::number( this->simulated_graph_number ) );
		//ui.customPlot->replot();
		//return;
	}

	// Get data from text entries
	std::string semiconductor_selected = ui.semiconductorType_comboBox->currentText().toStdString();
	double semiconductor_alloy_composition = ui.alloyComposition_doubleSpinBox->value();
	bool properly_formatted = false;
	double semiconductor_doping = ui.semiconductorDoping_lineEdit->text().toDouble( &properly_formatted );
	if( !properly_formatted )
		return;
	std::string insulator_selected = ui.insulatorType_comboBox->currentText().toStdString();
	double insulator_thickness = 1E-9 * ui.insulatorThickness_doubleSpinBox->value(); // Convert from nm to meters
	double temperature_in_k = ui.simulatedTemperature_doubleSpinBox->value();
	double interface_charge = ui.interfaceCharge_lineEdit->text().toDouble( &properly_formatted );
	std::string metal_selected = ui.metalType_comboBox->currentText().toStdString();

	std::map<std::string, std::function< Semiconductor()> > semiconductors = {
		{ "HgCdTe", [=] { return Semiconductor( HgCdTe( semiconductor_alloy_composition, temperature_in_k ), semiconductor_doping, temperature_in_k ); } }
	};
	std::map<std::string, std::function< Insulator()> > insulators = {
		{ "ZnS",  [=] { return Insulator( ZnS,                      insulator_thickness, interface_charge ); } },
		{ "CdTe", [=] { return Insulator( CdTe( temperature_in_k ), insulator_thickness, interface_charge ); } },
		{ "ZnO",  [=] { return Insulator( ZnO,                      insulator_thickness, interface_charge ); } },
		{ "Al2O3", [=] { return Insulator( Al2O3,                     insulator_thickness, interface_charge ); } }
	};
	std::map<std::string, std::function< Metal()> > metals = {
		{ "Aluminum"  , [=] { return Aluminum; } },
		{ "Chromium"  , [=] { return Chromium; } },
		{ "Gold"      , [=] { return Gold;       } },
		{ "Indium"    , [=] { return Indium;     } },
		{ "Molybdenum", [=] { return Molybdenum; } },
		{ "Nickel"    , [=] { return Nickel;     } },
		{ "Platinum"  , [=] { return Platinum;   } },
		{ "Titanium"  , [=] { return Titanium;   } },
	};
	Semiconductor semiconductor = semiconductors[ semiconductor_selected ]();
	Insulator insulator = insulators[ insulator_selected ]();
	Metal metal = metals[ metal_selected ]();

	// Output calculated material values
	ui.intrinsicCarrierConcentration_lineEdit->setText( QString::number( semiconductor.n_i, 'E', 2 ) + " cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B3 ) ) );
	ui.dielectricConstant_lineEdit->setText( QString::number( semiconductor.eps_s, 'G', 4 ) );
	ui.bandgap_lineEdit->setText( QString::number( semiconductor.bandgap, 'G', 4 ) + " eV" );
	ui.wavelength_lineEdit->setText( QString::number( 1.23984198406 / semiconductor.bandgap, 'G', 4 ) + " " + QString( QChar( 0x03BC ) ) + "m" );
	ui.semiconductorAffinity_lineEdit->setText( QString::number( semiconductor.affinity, 'G', 4 ) + " eV" );
	ui.semiconductorWorkFunction_lineEdit->setText( QString::number( semiconductor.work_function, 'G', 4 ) + " eV" );
	ui.metalWorkFunction_lineEdit->setText( QString::number( metal.work_function, 'G', 4 ) +" eV" );

	//double lower_bound = std::max( 0.0, ui.customPlot->xAxis->range().lower );
	//double upper_bound = std::max( 0.0, ui.customPlot->xAxis->range().upper );
	if( ui.displayHFPreview_checkBox->isChecked() )
	{
		auto[ voltages, capacitances ] = Get_MOS_Capacitance( semiconductor, insulator, metal, temperature_in_k, ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper, 1000.0 );
		capacitances /= insulator.capacitance;
		//ui.customPlot->Graph( toQVec( voltages ), toQVec( capacitances ), "HF Simulation" + QString::number( this->simulated_graph_number ),
		//							QString( "HF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(), QString::number( insulator_thickness * 1E9, 'f', 2 ),
		//												QString::number( temperature_in_k, 'f', 2 ) ), true );
	}
	if( ui.displayLFPreview_checkBox->isChecked() )
	{
		auto[ voltages, capacitances ] = Get_MOS_Capacitance( semiconductor, insulator, metal, temperature_in_k, ui.customPlot->xAxis->range().lower, ui.customPlot->xAxis->range().upper, 0.0 );
		capacitances /= insulator.capacitance;
		//ui.customPlot->Graph( toQVec( voltages ), toQVec( capacitances ), "LF Simulation" + QString::number( this->simulated_graph_number ),
		//							QString( "LF %1, t=%2 nm, T=%3 K" ).arg( ui.insulatorType_comboBox->currentText(), QString::number( insulator_thickness * 1E9, 'f', 2 ),
		//												QString::number( temperature_in_k, 'f', 2 ) ), true );
	}

	ui.customPlot->replot();
}

void IV_Plotter::Add_Mouse_Position_Label()
{
	QLabel* statusLabel = new QLabel( this );
	statusLabel->setText( "Status Label" );
	ui.statusBar->addPermanentWidget( statusLabel );
	connect( ui.customPlot, &QCustomPlot::mouseMove, [this, statusLabel]( QMouseEvent *event )
	{
		double x = ui.customPlot->xAxis->pixelToCoord( event->pos().x() );
		double y = ui.customPlot->yAxis->pixelToCoord( event->pos().y() );

		statusLabel->setText( QString( "%1 , %2" ).arg( x ).arg( y ) );
	} );
}

void IV_Plotter::treeContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );
	auto selected = ui.treeWidget->selectedItems();
	auto actually_clicked = ui.treeWidget->itemAt( pos );
	//if( selected.size() == 1 && ui.treeWidget->Get_Bottom_Children_Elements_Under( selected[ 0 ] ).size() == 1 ) // context menu on legend requested
	//{
	//	menu->addAction( "Set As Background", [this, selected]
	//	{
	//	} );
	//}

	menu->addAction( "Graph Selected", [this, selected]
	{
		for( const auto & tree_item : selected )
		{
			vector<const QTreeWidgetItem*> things_to_graph = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

			for( const QTreeWidgetItem* x : things_to_graph )
			{
				Graph_Tree_Node( x );
			}
		}
	} );

	menu->addAction( "Save to csv file", [this, selected]
	{
		vector<const QTreeWidgetItem*> all_things_to_save;
		for( const auto & tree_item : selected )
		{
			vector<const QTreeWidgetItem*> things_to_save = ui.treeWidget->Get_Bottom_Children_Elements_Under( tree_item );

			all_things_to_save.insert( all_things_to_save.end(), things_to_save.begin(), things_to_save.end() );
		}

		//this->Save_To_CSV( all_things_to_save );
	} );

	menu->popup( ui.treeWidget->mapToGlobal( pos ) );
}

void IV_Plotter::Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, Metadata meta, QString data_title )
{
	ui.customPlot->Graph( x_data, y_data, measurement_id, meta, data_title, true );
	ui.customPlot->replot();
}
