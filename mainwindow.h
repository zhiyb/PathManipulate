#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);

private slots:
	void browseListFile();
	void removeOutput();
	void loadListFile();
	void saveOutput();
	void generateList();
	void updatePath();

private:
	void appendList(QString path, QString filePath, QTreeWidgetItem *item);

	QLineEdit *listFile, *root, *filter;
	QTreeWidget *tree, *outputList;
	QLabel *path;
};

#endif // MAINWINDOW_H
