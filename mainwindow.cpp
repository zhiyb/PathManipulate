//#include <QtWebKit>
//#include <QtWebKitWidgets>
#include <QDebug>
#include <QFileInfo>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	QSplitter *splt = new QSplitter;
	splt->setChildrenCollapsible(false);
	setCentralWidget(splt);
	QWidget *w;
	splt->addWidget(w = new QWidget);
	QVBoxLayout *llay = new QVBoxLayout(w);
	splt->addWidget(w = new QWidget);
	QVBoxLayout *rlay = new QVBoxLayout(w);
	QPushButton *pb;
	QGridLayout *slay;
	QHBoxLayout *hlay;

	// Settings
	llay->addLayout(slay = new QGridLayout, 1);
	slay->addWidget(new QLabel(tr("List file:")),		0, 0);
	slay->addWidget(listFile = new QLineEdit,		0, 1);
	slay->addWidget(pb = new QPushButton(tr("...")),	0, 2);
	connect(pb, SIGNAL(clicked(bool)), this, SLOT(browseListFile()));
	slay->addWidget(new QLabel(tr("Filter:")),		1, 0);
	slay->addWidget(filter = new QLineEdit,			1, 1, 1, 2);
	slay->addWidget(pb = new QPushButton(tr("Load")),	2, 0, 1, 3);
	connect(pb, SIGNAL(clicked(bool)), this, SLOT(loadListFile()));

	// Tree view
	llay->addWidget(tree = new QTreeWidget, 2);
	tree->setHeaderLabel("Path");
	connect(tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(updatePath()));

	// Right layout
	rlay->addLayout(slay = new QGridLayout);
	slay->addWidget(new QLabel(tr("Root:")),		0, 0);
	slay->addWidget(root = new QLineEdit,			0, 1, 1, 2);

	// Output list
	rlay->addWidget(path = new QLabel);
	rlay->addLayout(hlay = new QHBoxLayout);
	hlay->addWidget(pb = new QPushButton(tr("Append")));
	connect(pb, SIGNAL(clicked(bool)), this, SLOT(generateList()));
	hlay->addWidget(pb = new QPushButton(tr("Remove")));
	connect(pb, SIGNAL(clicked(bool)), this, SLOT(removeOutput()));
	hlay->addWidget(pb = new QPushButton(tr("Save")));
	connect(pb, SIGNAL(clicked(bool)), this, SLOT(saveOutput()));
	rlay->addWidget(outputList = new QTreeWidget);
	QStringList list;
	list.append("URL");
	list.append("Path");
	outputList->setHeaderLabels(list);
	outputList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	outputList->setSelectionBehavior(QAbstractItemView::SelectRows);
	outputList->setSelectionMode(QAbstractItemView::MultiSelection);
}

void MainWindow::browseListFile()
{
	QString file = QFileDialog::getOpenFileName(this, tr("Select list file"));
	if (!file.isEmpty())
		listFile->setText(file);
}

void MainWindow::removeOutput()
{
	QList<QTreeWidgetItem *> list = outputList->selectedItems();
	while (!list.isEmpty())
		delete list.takeFirst();
}

void MainWindow::loadListFile()
{
	qDebug() << "Loading list file:" << listFile->text();

	QFile lf(listFile->text());
	if (!lf.open(QFile::ReadOnly)) {
		qWarning("Cannot open list file!");
		return;
	}
	QTextStream ts(&lf);
	ts.setCodec("UTF-8");
	if (ts.status() != QTextStream::Ok) {
		lf.close();
		qWarning("Cannot construct text stream!");
		return;
	}

	tree->clear();
	QTreeWidgetItem *item = tree->invisibleRootItem();
	while (!ts.atEnd()) {
		QString line = ts.readLine();
		if (line.isEmpty())
			continue;
		if (!filter->text().isEmpty())
			if (QRegExp(filter->text(), Qt::CaseInsensitive).indexIn(line) == -1)
				continue;
		QStringList lv = line.split('/', QString::SkipEmptyParts);
		// Extract parent path
		QList<QTreeWidgetItem *> list;
		while (item && item != tree->invisibleRootItem()) {
			list.push_front(item);
			item = item->parent();
		}
		while (!lv.isEmpty()) {
			QString str = lv.takeFirst();
			// TODO: parent folder .. support
			if (str.isEmpty() || str == ".")
				continue;
			// Remove common path from parent path
			if (!list.isEmpty()) {
				QTreeWidgetItem *it = list.takeFirst();
				if (it->text(0) == str) {
					item = it;
					continue;
				} else
					list.clear();
			}
			// Find entry
			if (!item)
				item = tree->invisibleRootItem();
			int i;
			for (i = 0; i < item->childCount(); i++)
				if (item->child(i)->text(0) == str)
					break;
			if (i == item->childCount())
				item = new QTreeWidgetItem(item, QStringList(str));
			else
				item = item->child(i);
		}
	}

	// Sort entries
	tree->sortByColumn(0, Qt::AscendingOrder);

	// Add file icons
	QFileIconProvider fip;
	QMap<QString, QIcon> iconMap;
	QTreeWidgetItemIterator it(tree->invisibleRootItem());
	while (*++it)		// Ignore root
		if ((*it)->childCount())
			(*it)->setIcon(0, fip.icon(fip.Folder));
		else {
			QFileInfo fi((*it)->text(0));
			QString sfx = fi.suffix();
#if 0	// FIXME: File icon retrieval not working
			QIcon icon = iconMap[sfx];
			if (icon.isNull()) {
				QTemporaryFile tf(QDir::tempPath() + "/XXXXXX." + sfx);
				tf.open();
				iconMap[sfx] = icon = fip.icon(fip.File/*QFileInfo(tf)*/);
			}
			(*it)->setIcon(0, icon);
#else
			iconMap[sfx] = fip.icon(fip.File);
#endif
		}

	qDebug() << "List file loaded.";
	lf.close();
}

void MainWindow::saveOutput()
{
	QString file = QFileDialog::getSaveFileName(this, tr("Save list to..."), QString(), tr("List (*.list)"));
	if (file.isEmpty())
		return;

	QFile lf(file);
	if (!lf.open(QFile::WriteOnly)) {
		qWarning("Open file for write failed!");
		return;
	}
	QTextStream ts(&lf);
	ts.setCodec("UTF-8");
	if (ts.status() != QTextStream::Ok) {
		lf.close();
		qWarning("Cannot construct text stream!");
		return;
	}

	QTreeWidgetItemIterator it(outputList->invisibleRootItem());
	while (*++it)		// Ignore root
		ts << "\"" << (*it)->text(0) << "\" \"" << (*it)->text(1) << "\"\n";

	lf.close();
}

void MainWindow::generateList()
{
	appendList(root->text() + path->text(), "." + path->text(), tree->currentItem());
}

void MainWindow::updatePath()
{
	QTreeWidgetItem *item = tree->currentItem();
	if (!item)
		return;
	QStringList list;
	while (item) {
		list.push_front(item->text(0));
		item = item->parent();
	}
	QString path;
	while (!list.isEmpty())
		path += "/" + list.takeFirst();
	this->path->setText(path);
}

void MainWindow::appendList(QString path, QString filePath, QTreeWidgetItem *item)
{
	if (!item)
		return;
	if (item->childCount()) {
		int i;
		for (i = 0; i < item->childCount(); i++)
			appendList(path + "/" + item->child(i)->text(0), \
				   filePath + "/" + item->child(i)->text(0), item->child(i));
	} else {
		QTreeWidgetItem *oItem = new QTreeWidgetItem(outputList);
		oItem->setText(0, path);
		oItem->setText(1, filePath);
		oItem->setIcon(0, item->icon(0));
		oItem->setIcon(1, item->icon(0));
	}
}
