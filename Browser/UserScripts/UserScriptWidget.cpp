#include "UserScriptWidget.h"
#include "ui_UserScriptWidget.h"
#include "BrowserApplication.h"
#include "UserScriptManager.h"
#include "UserScriptModel.h"

#include "CodeEditor.h"
#include "JavaScriptHighlighter.h"

#include <QMessageBox>
#include <QResizeEvent>
#include <algorithm>
#include <vector>

UserScriptWidget::UserScriptWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserScriptWidget)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    ui->tableViewScripts->setModel(sBrowserApplication->getUserScriptManager()->getModel());

    connect(ui->tableViewScripts, &UserScriptTableView::clicked, this, &UserScriptWidget::onItemClicked);
    connect(ui->pushButtonDeleteScript, &QPushButton::clicked, this, &UserScriptWidget::onDeleteButtonClicked);
    connect(ui->pushButtonEditScript, &QPushButton::clicked, this, &UserScriptWidget::onEditButtonClicked);
}

void UserScriptWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int tableWidth = ui->tableViewScripts->geometry().width();
    ui->tableViewScripts->setColumnWidth(0, tableWidth / 25);
    ui->tableViewScripts->setColumnWidth(1, tableWidth / 4);
    ui->tableViewScripts->setColumnWidth(2, tableWidth / 2);
    ui->tableViewScripts->setColumnWidth(3, tableWidth / 5);
}

UserScriptWidget::~UserScriptWidget()
{
    delete ui;
}

void UserScriptWidget::onItemClicked(const QModelIndex &/*index*/)
{
    ui->pushButtonEditScript->setEnabled(true);
    ui->pushButtonDeleteScript->setEnabled(true);
}

void UserScriptWidget::onDeleteButtonClicked()
{
    QModelIndexList selection = ui->tableViewScripts->selectionModel()->selectedIndexes();
    if (selection.empty())
        return;

    // Get user confirmation before deleting items
    QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to delete the selected script(s)?"),
                                  QMessageBox::Yes|QMessageBox::No);
    if (answer == QMessageBox::No)
        return;

    // Get rows from selection without duplicates (in case multiple columns of same row(s) are selected)
    std::vector<int> selectedRows;
    for (const QModelIndex &idx : selection)
    {
        int row = idx.row();
        auto it = std::find(selectedRows.begin(), selectedRows.end(), row);
        if (it == selectedRows.end())
            selectedRows.push_back(row);
    }
    // Sort rows in descending order
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());

    // Remove scripts
    UserScriptModel *model = qobject_cast<UserScriptModel*>(ui->tableViewScripts->model());
    for (int row : selectedRows)
        model->removeRow(row);
}

void UserScriptWidget::onEditButtonClicked()
{
    QModelIndex idx = ui->tableViewScripts->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    //TODO: have dedicated widget for this, with a save and cancel button at the top or bottom
    UserScriptModel *model = qobject_cast<UserScriptModel*>(ui->tableViewScripts->model());
    QModelIndex nameIdx = model->index(idx.row(), 1);
    QString scriptName = model->data(nameIdx, Qt::DisplayRole).toString();
    QString source = model->getScriptSource(idx.row());

    CodeEditor *editor = new CodeEditor;
    editor->setPlainText(source);
    JavaScriptHighlighter *j = new JavaScriptHighlighter;
    j->setDocument(editor->document());
    editor->setWindowTitle(tr("Editing %1").arg(scriptName));
    editor->setMinimumWidth(640);
    editor->setMinimumHeight(geometry().height());
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->show();
}
