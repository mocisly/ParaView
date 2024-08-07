// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginManager.h"
#include "ui_pqPluginEULADialog.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqQtDeprecated.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "vtkPVLogger.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginsInformation.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPluginManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QCoreApplication>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QTextStream>

#include <cassert>
#include <sstream>

class pqPluginManager::pqInternals
{
public:
  QSet<QString> LocalHiddenPlugins;
  QSet<QString> RemoteHiddenPlugins;
  QList<QPointer<pqServer>> Servers;

  QString getXML(vtkPVPluginsInformation* info, bool remote)
  {
    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" ?>\n";
    stream << "<Plugins>\n";
    for (unsigned int cc = 0; cc < info->GetNumberOfPlugins(); cc++)
    {
      if ((remote && this->RemoteHiddenPlugins.contains(info->GetPluginFileName(cc))) ||
        (!remote && this->LocalHiddenPlugins.contains(info->GetPluginFileName(cc))))
      {
        continue;
      }

      stream << "  <Plugin name=\"" << info->GetPluginName(cc) << "\""
             << " filename=\"" << info->GetPluginFileName(cc) << "\""
             << " auto_load=\"" << (info->GetAutoLoad(cc) ? 1 : 0) << "\""
             << " delayed_load=\"" << (info->GetDelayedLoad(cc) ? 1 : 0) << "\""
             << " version=\"" << info->GetPluginVersion(cc) << "\""
             << " description=\"" << info->GetDescription(cc) << "\"";

      std::vector<std::string> xmls = info->GetXMLs(cc);
      if (!xmls.empty())
      {
        stream << " >\n";
        for (std::string xml : xmls)
        {
          stream << "<XML filename=\"" << xml << "\" />\n";
        }
        stream << "</Plugin>\n";
      }
      else
      {
        stream << " />\n";
      }
    }
    stream << "</Plugins>\n";
    // cout << stream.str().c_str() << endl;
    return QString(stream.str().c_str());
  }
};

//=============================================================================
static QString pqPluginManagerSettingsKeyForRemote(pqServer* server)
{
  assert(server && server->isRemote());
  // locate the xml-config from settings associated with this server and ask
  // the server to parse it.
  const pqServerResource& resource = server->getResource();
  QString uri = resource.configuration().isNameDefault() ? resource.schemeHostsPorts().toURI()
                                                         : resource.configuration().name();
  QString key = QString("/PluginsList/%1:%2").arg(uri).arg(QCoreApplication::applicationFilePath());
  return key;
}

//=============================================================================
static QString pqPluginManagerSettingsKeyForLocal()
{
  return QString("/PluginsList/Local:%1").arg(QCoreApplication::applicationFilePath());
}

//-----------------------------------------------------------------------------
pqPluginManager::pqPluginManager(QObject* parentObject)
  : Superclass(parentObject)
{
  // setup EULA confirmation callback. Note that is still too late for auto-load
  // plugins. For auto-load plugins, the EULA is always auto-accepted.
  vtkPVPlugin::SetEULAConfirmationCallback(pqPluginManager::confirmEULA);

  this->Internals = new pqInternals();

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  // we ensure that the auto-load plugins are loaded before the application
  // realizes that a new server connection has been made.
  // (BUG #12238).
  QObject::connect(
    smmodel, SIGNAL(serverReady(pqServer*)), this, SLOT(loadPluginsFromSettings(pqServer*)));
  QObject::connect(
    smmodel, SIGNAL(serverRemoved(pqServer*)), this, SLOT(onServerDisconnected(pqServer*)));

  // After the new server has been setup, we can validate if the plugin
  // requirements have been met successfully.
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(finishedAddingServer(pqServer*)), this, SLOT(onServerConnected(pqServer*)));

  // observer plugin loaded events from PluginManager to detect plugins loaded
  // from Python or otherwise.
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  mgr->AddObserver(
    vtkSMPluginManager::PluginLoadedEvent, this, &pqPluginManager::updatePluginLists);
}

//-----------------------------------------------------------------------------
pqPluginManager::~pqPluginManager()
{
  // save all settings for each open server session.
  Q_FOREACH (pqServer* server, this->Internals->Servers)
  {
    this->onServerDisconnected(server);
  }
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadPluginsFromSettings()
{
  // Load local plugins information and then load those plugins.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = pqPluginManagerSettingsKeyForLocal();
  QString local_plugin_config = settings->value(key).toString();
  if (!local_plugin_config.isEmpty())
  {
    vtkVLogScopeF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Loading local Plugin configuration using settings key: %s", key.toUtf8().data());
    vtkSMProxyManager::GetProxyManager()->GetPluginManager()->LoadPluginConfigurationXMLFromString(
      local_plugin_config.toUtf8().data(), nullptr, false);
  }
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadPluginsFromSettings(pqServer* server)
{
  // Tell the server to load all default-plugins.
  if (server && server->isRemote())
  {
    // locate the xml-config from settings associated with this server and ask
    // the server to parse it.
    QString key = pqPluginManagerSettingsKeyForRemote(server);
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QString remote_plugin_config = settings->value(key).toString();
    // now pass this xml to the vtkPVPluginTracker on the remote
    // processes.
    if (!remote_plugin_config.isEmpty())
    {
      vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
        "Loading remote Plugin configuration using settings key: %s", key.toUtf8().data());
      vtkSMProxyManager::GetProxyManager()
        ->GetPluginManager()
        ->LoadPluginConfigurationXMLFromString(
          remote_plugin_config.toUtf8().data(), server->session(), true);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerConnected(pqServer* server)
{
  this->Internals->Servers.push_back(server);
  this->updatePluginLists();

  // Validate plugins i.e. check plugins that are required on client and server
  // are indeed present on both.
  if (!this->verifyPlugins(server))
  {
    Q_EMIT this->requiredPluginsNotLoaded(server);
  }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerDisconnected(pqServer* server)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (server && server->isRemote())
  {
    QString remoteKey = pqPluginManagerSettingsKeyForRemote(server);
    // locate the xml-config from settings associated with this server and ask
    // the server to parse it.
    settings->setValue(
      remoteKey, this->Internals->getXML(this->loadedExtensions(server, true), true));
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Saving remote Plugin configuration using settings key: %s", remoteKey.toUtf8().data());
  }

  // just save the local plugin info to be on the safer side.
  QString key = pqPluginManagerSettingsKeyForLocal();
  settings->setValue(key, this->Internals->getXML(this->loadedExtensions(server, false), false));
  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
    "Saving local Plugin configuration using settings key: %s", key.toUtf8().data());

  this->Internals->Servers.removeAll(server);
}

//-----------------------------------------------------------------------------
void pqPluginManager::updatePluginLists()
{
  Q_EMIT this->pluginsUpdated();
}

//-----------------------------------------------------------------------------
vtkPVPluginsInformation* pqPluginManager::loadedExtensions(pqServer* session, bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  return (remote && session && session->isRemote()) ? mgr->GetRemoteInformation(session->session())
                                                    : mgr->GetLocalInformation();
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadExtension(
  pqServer* server, const QString& plugin, QString* vtkNotUsed(errorMsg), bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();

  bool ret_val = false;
  if (remote && server && server->isRemote())
  {
    ret_val = mgr->LoadRemotePlugin(plugin.toUtf8().data(), server->session());
  }
  else
  {
    // All Load*Plugin* call need a utf8 encoded filename or
    // xmlcontent, since vtksys::DynamicLoader itself takes care
    // Of converting to local8bit, even locally.
    ret_val = mgr->LoadLocalPlugin(plugin.toUtf8().data());
  }

  return ret_val ? LOADED : NOTLOADED;
}

//-----------------------------------------------------------------------------
QStringList pqPluginManager::pluginPaths(pqServer* session, bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  QString paths =
    remote ? mgr->GetRemotePluginSearchPaths(session->session()) : mgr->GetLocalPluginSearchPaths();
  return paths.split(';', PV_QT_SKIP_EMPTY_PARTS);
}

//-----------------------------------------------------------------------------
void pqPluginManager::addPluginConfigFile(pqServer* server, const QString& config, bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  mgr->LoadPluginConfigurationXML(config.toUtf8().data(), server->session(), remote);
}

//-----------------------------------------------------------------------------
void pqPluginManager::hidePlugin(const QString& lib, bool remote)
{
  if (remote)
  {
    this->Internals->RemoteHiddenPlugins.insert(lib);
  }
  else
  {
    this->Internals->LocalHiddenPlugins.insert(lib);
  }
}

//-----------------------------------------------------------------------------
bool pqPluginManager::isHidden(const QString& lib, bool remote)
{
  return remote ? this->Internals->RemoteHiddenPlugins.contains(lib)
                : this->Internals->LocalHiddenPlugins.contains(lib);
}

//-----------------------------------------------------------------------------
bool pqPluginManager::verifyPlugins(pqServer* activeServer)
{
  if (!activeServer || !activeServer->isRemote())
  {
    // no verification needed for non-remote servers.
    return true;
  }

  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  return mgr->FulfillPluginRequirements(activeServer->session());
}

//-----------------------------------------------------------------------------
bool pqPluginManager::confirmEULA(vtkPVPlugin* plugin)
{
  assert(plugin->GetEULA() != nullptr);

  pqSettings* settings = pqApplicationCore::instance()->settings();

  QString pluginKey;
  QTextStream(&pluginKey) << "EULAConfirmation-" << plugin->GetPluginName() << "-"
                          << plugin->GetPluginVersionString() << "-Confirmed";
  if (settings->value(pluginKey, false).toBool() == true)
  {
    // previously accepted.
    return true;
  }

  QDialog dialog(pqCoreUtilities::mainWidget());
  Ui::PluginEULADialog ui;
  ui.setupUi(&dialog);
  ui.textEdit->setHtml(
    QString("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
            "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
            "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
            "p, li { white-space: pre-wrap; }\n"
            "</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; "
            "font-weight:400; font-style:normal;\">\n"
            "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; "
            "-qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">EULA for "
            "plugin</span></p></body></html>"));

  ui.buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Accept"));
  ui.buttonBox->button(QDialogButtonBox::No)->setText(tr("Decline"));
  ui.buttonBox->button(QDialogButtonBox::No)->setDefault(true);

  dialog.setWindowTitle(tr("End User License Agreement for '%1'").arg(plugin->GetPluginName()));
  ui.textEdit->setText(plugin->GetEULA());

  if (dialog.exec() == QDialog::Accepted)
  {
    settings->setValue(pluginKey, true);
    return true;
  }

  return false;
}
