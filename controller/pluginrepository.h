#ifndef PLUGINREPOSITORY_H
#define PLUGINREPOSITORY_H

#include <QList>
#include <QString>
#include <optional>

#include "plugininfo.h"
#include "types.h"

#if defined(OI_MAIN_LIB)
#  define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#  define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

/*!
 * \brief The PluginRepository class
 * Scans the system (and user) plugin directories once and exposes the result
 * as plain domain objects (see plugininfo.h) - no consumer needs to know this
 * used to be backed by SystemDbManager's SQLite catalog.
 */
class OI_MAIN_EXPORT PluginRepository
{
public:

    static void init();

    //#################
    //get all / by name
    //#################

    static QList<sys::Plugin> getAll();
    static std::optional<sys::Plugin> getByName(const QString &name);          //convenience/display lookup - can be ambiguous, see plugininfo.h
    static std::optional<sys::Plugin> getByFilePath(const QString &filePath);  //the real, collision-safe lookup

    //#############
    //get functions
    //#############

    static QList<sys::Function> getAllFunctions();
    //whether a function is a "create" function for the given feature type, i.e. one
    //that can produce it from scratch. A pure predicate over the function itself -
    //exposed so consumers filtering their own function lists (e.g. proxy models over
    //an injected list) can apply identical semantics without querying this repository
    static bool isCreateFunctionFor(const sys::Function &function, const FeatureTypes &type);

    static QList<sys::Function> getFunctionsApplicableTo(const FeatureTypes &type);   //fit/construct/special/system-transformation functions, i.e. functions used to create a feature
    static QList<sys::Function> getChangeFunctions(const FeatureTypes &type);         //everything else, i.e. functions used to modify an already-solved feature
    static std::optional<sys::Function> getByQualifiedId(const QString &qualifiedId);

    //################
    //default function
    //################

    //resolves Settings' "defaultFunction.<type>" (Project -> User -> System) against
    //getFunctionsApplicableTo(type) by name - the same convenience-lookup semantics
    //as getByName(), and falls through to a less specific tier if a more specific
    //one's reference no longer resolves (e.g. a project saved against a plugin set
    //that's since changed)
    static std::optional<sys::Function> getDefaultFunction(const FeatureTypes &type);

    //#########
    //get tools
    //#########

    static QList<sys::Tool> getAllTools();

private:

    static QList<sys::Plugin> scanDirectory(const QString &dirPath);
    static sys::Plugin scanPluginFile(const QString &filePath);

    static QList<sys::Plugin> plugins;
    static bool isInit;

};

#endif // PLUGINREPOSITORY_H
