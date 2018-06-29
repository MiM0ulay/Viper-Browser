#ifndef URLSUGGESTIONLISTMODEL_H
#define URLSUGGESTIONLISTMODEL_H

#include <vector>
#include <QAbstractListModel>
#include <QIcon>
#include <QString>

/**
 * @struct URLSuggestion
 * @brief Container for the information contained in a single row of data in the \ref URLSuggestionListModel
 */
struct URLSuggestion
{
    QIcon Favicon;
    QString Title;
    QString URL;

    URLSuggestion() = default;

    URLSuggestion(const QIcon &icon, const QString &title, const QString &url);
};

/**
 * @class URLSuggestionListModel
 * @brief Contains a list of URLs to be suggested to the user as they
 *        type into the \ref URLLineEdit
 */
class URLSuggestionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        Favicon = Qt::UserRole + 1,
        Title   = Qt::UserRole + 2,
        Link    = Qt::UserRole + 3
    };

public:
    /// Constructs the URL suggestion list model with the given parent
    explicit URLSuggestionListModel(QObject *parent = nullptr);

    /// Returns the number of rows under the given parent
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data stored under the given role for the item referred to by the index.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    /// Sets the suggested items to be displayed in the model
    void setSuggestions(std::vector<URLSuggestion> suggestions);

private:
    /// Contains suggested URLs based on the current input
    std::vector<URLSuggestion> m_suggestions;
};

#endif // URLSUGGESTIONLISTMODEL_H
