#include "pluginrepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QJsonObject>

#include "pluginloader.h"
#include "oimetadata.h"

QList<sys::Plugin> PluginRepository::plugins;
bool PluginRepository::isInit = false;

/*!
 * \brief PluginRepository::init
 * Scans <appDir>/plugins/ once and caches the result for the session.
 */
void PluginRepository::init(){

    PluginRepository::plugins.clear();

    QString systemPluginDir = qApp->applicationDirPath() + "/plugins";
    PluginRepository::plugins.append(PluginRepository::scanDirectory(systemPluginDir));

    PluginRepository::isInit = true;

}

/*!
 * \brief PluginRepository::scanDirectory
 * \param dirPath
 * \return every valid OpenIndy plugin found directly in dirPath
 */
QList<sys::Plugin> PluginRepository::scanDirectory(const QString &dirPath){

    QList<sys::Plugin> result;

    QDir dir(dirPath);
    if(!dir.exists()){
        return result;
    }

    foreach(const QString &fileName, dir.entryList(QStringList() << "p_*.dll" << "libp_*.so", QDir::Files)){

        QString filePath = dir.absoluteFilePath(fileName);

        QPluginLoader probe(filePath);
        bool isOiPlugin = probe.metaData().value("MetaData").toObject().value("isOiPlugin").toBool();
        if(!isOiPlugin){
            continue;
        }

        result.append(PluginRepository::scanPluginFile(filePath));

    }

    return result;

}

/*!
 * \brief PluginRepository::scanPluginFile
 * Instantiates every Function/Sensor/Tool/Exchange/Simulation a plugin file
 * offers and calls init() on each - the same work PluginCopier::savePlugin()
 * used to do once at import time to populate the DB, now done at startup.
 * \param filePath
 * \return
 */
sys::Plugin PluginRepository::scanPluginFile(const QString &filePath){

    QPluginLoader pluginLoader(filePath);
    QJsonObject meta = pluginLoader.metaData().value("MetaData").toObject();

    QString name = meta.value("name").toString();
    QString description = meta.value("description").toString();
    QString version = meta.value("pluginVersion").toString();
    QString author = meta.value("author").toString();

    //lightweight owner reference embedded into each function/sensor/etc below -
    //empty child lists, since a function shouldn't carry a copy of its plugin's
    //entire other contents
    sys::Plugin ownerRef(name, description, version, author, filePath,
                         QList<sys::Function>(), QList<sys::Sensor>(), QList<sys::Tool>(),
                         QList<sys::Exchange>(), QList<sys::Simulation>());

    QList<sys::Function> functions;
    foreach(const QPointer<Function> &f, PluginLoader::loadFunctionPlugins(filePath)){
        if(f.isNull()){ continue; }
        QList<ElementTypes> neededElements;
        foreach(const NeededElement &ne, f->getNeededElements()){
            neededElements.append(ne.typeOfElement);
        }
        functions.append(sys::Function(f->getMetaData().name, f->getMetaData().description, ownerRef,
                                        f->getApplicableFor(), neededElements, f->getMetaData().iid));
    }

    QList<sys::Sensor> sensors;
    foreach(const QPointer<Sensor> &s, PluginLoader::loadSensorPlugins(filePath)){
        if(s.isNull()){ continue; }
        sensors.append(sys::Sensor(s->getMetaData().name, s->getMetaData().description, ownerRef, s->getMetaData().iid));
    }

    QList<sys::Tool> tools;
    foreach(const QPointer<Tool> &t, PluginLoader::loadToolPlugins(filePath)){
        if(t.isNull()){ continue; }
        tools.append(sys::Tool(t->getMetaData().name, t->getMetaData().description, ownerRef));
    }

    QList<sys::Exchange> exchanges;
    foreach(const QPointer<ExchangeSimpleAscii> &e, PluginLoader::loadExchangeSimpleAsciiPlugins(filePath)){
        if(e.isNull()){ continue; }
        exchanges.append(sys::Exchange(e->getMetaData().name, e->getMetaData().description, ownerRef, e->getMetaData().iid));
    }
    foreach(const QPointer<ExchangeDefinedFormat> &e, PluginLoader::loadExchangeDefinedFormatPlugins(filePath)){
        if(e.isNull()){ continue; }
        exchanges.append(sys::Exchange(e->getMetaData().name, e->getMetaData().description, ownerRef, e->getMetaData().iid));
    }

    QList<sys::Simulation> simulations;
    foreach(const QPointer<SimulationModel> &sim, PluginLoader::loadSimulationPlugins(filePath)){
        if(sim.isNull()){ continue; }
        simulations.append(sys::Simulation(sim->getMetaData().name, sim->getMetaData().description, ownerRef));
    }

    return sys::Plugin(name, description, version, author, filePath, functions, sensors, tools, exchanges, simulations);

}

/*!
 * \brief PluginRepository::getAll
 * \return
 */
QList<sys::Plugin> PluginRepository::getAll(){
    if(!PluginRepository::isInit){ PluginRepository::init(); }
    return PluginRepository::plugins;
}

/*!
 * \brief PluginRepository::getByName
 * Convenience/display lookup - first match wins if two plugins share a name.
 * \param name
 * \return
 */
std::optional<sys::Plugin> PluginRepository::getByName(const QString &name){
    foreach(const sys::Plugin &plugin, PluginRepository::getAll()){
        if(plugin.name().compare(name) == 0){
            return plugin;
        }
    }
    return std::nullopt;
}

/*!
 * \brief PluginRepository::getByFilePath
 * The real, collision-safe lookup - file paths can't collide.
 * \param filePath
 * \return
 */
std::optional<sys::Plugin> PluginRepository::getByFilePath(const QString &filePath){
    foreach(const sys::Plugin &plugin, PluginRepository::getAll()){
        if(plugin.filePath().compare(filePath) == 0){
            return plugin;
        }
    }
    return std::nullopt;
}

/*!
 * \brief PluginRepository::getAllFunctions
 * \return every function across every loaded plugin
 */
QList<sys::Function> PluginRepository::getAllFunctions(){
    QList<sys::Function> result;
    foreach(const sys::Plugin &plugin, PluginRepository::getAll()){
        result.append(plugin.functions());
    }
    return result;
}

/*!
 * \brief PluginRepository::getFunctionsApplicableTo
 * Functions used to CREATE a feature of the given type (fit/construct/special,
 * or system-transformation for trafo params) - mirrors SystemDbManager's old
 * getCreateFunctions() category split.
 * \param type
 * \return
 */
QList<sys::Function> PluginRepository::getFunctionsApplicableTo(const FeatureTypes &type){
    QList<sys::Function> result;
    foreach(const sys::Function &function, PluginRepository::getAllFunctions()){
        bool isCreateCategory;
        if(type == eTrafoParamFeature){
            isCreateCategory = function.interfaceCategory().compare(OiMetaData::iid_SystemTransformation) == 0;
        }else{
            isCreateCategory = function.interfaceCategory().compare(OiMetaData::iid_FitFunction) == 0
                    || function.interfaceCategory().compare(OiMetaData::iid_ConstructFunction) == 0
                    || function.interfaceCategory().compare(OiMetaData::iid_SpecialFunction) == 0;
        }
        if(isCreateCategory && function.applicableFor().contains(type)){
            result.append(function);
        }
    }
    return result;
}

/*!
 * \brief PluginRepository::getChangeFunctions
 * Functions used to CHANGE an already-solved feature of the given type -
 * everything that isn't a create-category function, per SystemDbManager's
 * old getChangeFunctions() split.
 * \param type
 * \return
 */
QList<sys::Function> PluginRepository::getChangeFunctions(const FeatureTypes &type){
    QList<sys::Function> result;
    foreach(const sys::Function &function, PluginRepository::getAllFunctions()){
        bool isCreateCategory = function.interfaceCategory().compare(OiMetaData::iid_SystemTransformation) == 0
                || function.interfaceCategory().compare(OiMetaData::iid_FitFunction) == 0
                || function.interfaceCategory().compare(OiMetaData::iid_ConstructFunction) == 0
                || function.interfaceCategory().compare(OiMetaData::iid_SpecialFunction) == 0;
        if(!isCreateCategory && function.applicableFor().contains(type)){
            result.append(function);
        }
    }
    return result;
}

/*!
 * \brief PluginRepository::getByQualifiedId
 * \param qualifiedId
 * \return
 */
std::optional<sys::Function> PluginRepository::getByQualifiedId(const QString &qualifiedId){
    foreach(const sys::Function &function, PluginRepository::getAllFunctions()){
        if(function.qualifiedId().compare(qualifiedId) == 0){
            return function;
        }
    }
    return std::nullopt;
}

/*!
 * \brief PluginRepository::getAllTools
 * \return
 */
QList<sys::Tool> PluginRepository::getAllTools(){
    QList<sys::Tool> result;
    foreach(const sys::Plugin &plugin, PluginRepository::getAll()){
        result.append(plugin.tools());
    }
    return result;
}
