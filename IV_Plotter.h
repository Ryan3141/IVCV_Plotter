#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_IV_Plotter.h"
#include "SQL_Manager.h"

class IV_Plotter : public QMainWindow
{
	Q_OBJECT

public:
	IV_Plotter(QWidget *parent = Q_NULLPTR);

private:
	void Add_Mouse_Position_Label();
	void Initialize_Tree_Table();
	void Initialize_Graph();
	void Graph_Tree_Node( const QTreeWidgetItem* tree_item );
	void treeContextMenuRequest( QPoint pos );
	void Graph( QString measurement_id, const QVector<double> & x_data, const QVector<double> & y_data, Metadata meta, QString data_title = QString() );
	void Update_Preview_Graph();

	int simulated_graph_number = 0;
	Ui::IV_PlotterClass ui;
	SQL_Manager* sql_manager;

	using XY_Data = std::tuple< QVector<double>, QVector<double> >;
	using Metadata = std::vector<QVariant>;
};
