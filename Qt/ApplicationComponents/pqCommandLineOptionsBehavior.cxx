// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCommandLineOptionsBehavior.h"

#include "pqActiveObjects.h"
#include "pqCollaborationEventPlayer.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreConfiguration.h"
#include "pqCoreUtilities.h"
#include "pqDeleteReaction.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqLiveInsituManager.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqQtDeprecated.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerConnectReaction.h"
#include "pqServerManagerModel.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqTimeKeeper.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include <vtksys/SystemTools.hxx>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <cassert>

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonInterpreter.h"
#endif

//-----------------------------------------------------------------------------
pqCommandLineOptionsBehavior::pqCommandLineOptionsBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  pqTimer::singleShot(100, this, SLOT(processCommandLineOptions()));
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processCommandLineOptions()
{
  // Handle server connection.
  this->processServerConnection();

  // Handle plugins to load at startup.
  this->processPlugins();

  // Handle data.
  this->processData();

  // Handle state file.
  this->processState();

  // Handle script.
  this->processScript();

  // Process live
  this->processLive();

  auto rcConfig = vtkRemotingCoreConfiguration::GetInstance();
  if (rcConfig->GetDisableRegistry())
  {
    // a cout for test playback.
    cout << "Process started" << endl;
  }

  // Process tests.
  const bool success = this->processTests();
  if (pqCoreConfiguration::instance()->exitApplicationWhenTestsDone())
  {
    if (pqCoreConfiguration::instance()->testMaster())
    {
      pqCollaborationEventPlayer::wait(1000);
    }

    // Make sure that the pqApplicationCore::prepareForQuit() method
    // get called
    QApplication::closeAllWindows();
    QApplication::instance()->exit(success ? 0 : 1);
  }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processServerConnection()
{
  auto rcConfig = vtkRemotingCoreConfiguration::GetInstance();

  // check for --server.
  const auto serverResourceName = QString::fromStdString(rcConfig->GetServerResourceName());

  // (server-url gets lower priority than --server).
  const auto serverURL = QString::fromStdString(rcConfig->GetServerURL());
  if (!serverResourceName.isEmpty())
  {
    if (!pqServerConnectReaction::connectToServerUsingConfigurationName(
          qPrintable(serverResourceName), false))
    {
      qCritical() << "Could not connect to requested server \"" << serverResourceName
                  << "\". Creating default builtin connection.";
    }
  }
  else if (!serverURL.isEmpty())
  {
    if (serverURL.indexOf('|') != -1)
    {
      // We should connect multiple times
      const QStringList urls = serverURL.split(QRegularExpression("\\|"), PV_QT_SKIP_EMPTY_PARTS);
      for (const QString& url : urls)
      {
        if (!pqServerConnectReaction::connectToServer(pqServerResource(url), false))
        {
          qCritical() << "Could not connect to requested server \"" << url
                      << "\". Creating default builtin connection.";
        }
      }
    }
    else if (!pqServerConnectReaction::connectToServer(pqServerResource(serverURL), false))
    {
      qCritical() << "Could not connect to requested server \"" << serverURL
                  << "\". Creating default builtin connection.";
    }
  }

  // Connect to builtin, if none present.
  if (pqActiveObjects::instance().activeServer() == nullptr)
  {
    pqServerConnectReaction::connectToServer(pqServerResource("builtin:"), false);
  }

  // Now we are assured that some default server connection has been made
  // (either the one requested by the user on the command line or simply the
  // default one).
  assert(pqActiveObjects::instance().activeServer() != nullptr);
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processData()
{
  auto cConfig = pqCoreConfiguration::instance();
  // check for --data option.
  if (cConfig->dataFileNames().empty())
  {
    return;
  }

  for (const auto& fname : cConfig->dataFileNames())
  {
    QString path = QString::fromUtf8(fname.c_str());

    // We don't directly set the data file name instead use the dialog. This
    // makes it possible to select a file group.
    // This also resolve relative path into a canonical one.
    pqFileDialog dialog(pqActiveObjects::instance().activeServer(), pqCoreUtilities::mainWidget(),
      tr("Internal Open File"), QString(), QString(), false);
    dialog.setFileMode(pqFileDialog::ExistingFiles);

    if (!dialog.selectFile(path))
    {
      qCritical() << "Cannot open data file \"" << path << "\"";
    }
    QList<QStringList> files = dialog.getAllSelectedFiles();
    QStringList file;
    Q_FOREACH (file, files)
    {
      if (pqLoadDataReaction::loadData(file) == nullptr)
      {
        qCritical() << "Failed to load data file: " << path;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processState()
{
  auto cConfig = pqCoreConfiguration::instance();
  std::string fullPath;
  if (!cConfig->stateFileName().empty())
  {
    fullPath = vtksys::SystemTools::CollapseFullPath(cConfig->stateFileName());
  }
  if (!fullPath.empty())
  {
    QFileInfo fileInfo(QString::fromStdString(fullPath));
    if (fileInfo.exists())
    {
      // Load state file using canonical path without fix-filenames dialog.
      pqLoadStateReaction::loadState(fileInfo.canonicalFilePath(), true);
    }
    else
    {
      qCritical() << "Specified state file does not exist: '" << fullPath.c_str() << "'";
    }
  }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processScript()
{
  auto cConfig = pqCoreConfiguration::instance();
  const auto& fname = cConfig->pythonScript();
  if (!fname.empty())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
    QFile file(QString::fromStdString(fname));
    if (file.open(QIODevice::ReadOnly))
    {
      QByteArray code = file.readAll();
      vtkPythonInterpreter::RunSimpleString(code.data());
    }
    else
    {
      qCritical() << "Cannot open Python script specified: '" << fname.c_str() << "'";
    }
#else
    qCritical() << "Python support not enabled. Cannot run python scripts.";
#endif
  }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processLive()
{
  auto cConfig = pqCoreConfiguration::instance();

  // check if a Catalyst Live port was passed in that we should automatically attempt
  // to establish a connection to.
  if (cConfig->catalystLivePort() != -1)
  {
    pqLiveInsituManager* insituManager = pqLiveInsituManager::instance();
    insituManager->connect(pqActiveObjects::instance().activeServer(), cConfig->catalystLivePort());
  }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processPlugins()
{
  // Load the test plugins specified at the command line
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMPluginManager* pluginManager = pxm->GetPluginManager();
  vtkSMSession* activeSession = pxm->GetActiveSession();

  const auto& plugins = vtkRemotingCoreConfiguration::GetInstance()->GetPlugins();
  for (const auto& plugin : plugins)
  {
    // Make in-code plugin XML
    auto xml =
      QString("<Plugins><Plugin name=\"%1\" auto_load=\"1\"/></Plugins>").arg(plugin.c_str());

    // Load the plugin into the plugin manager. Local and remote loading is done here.
    pluginManager->LoadPluginConfigurationXMLFromString(qPrintable(xml), activeSession, false);
    pluginManager->LoadPluginConfigurationXMLFromString(qPrintable(xml), activeSession, true);
  }
}

//-----------------------------------------------------------------------------
bool pqCommandLineOptionsBehavior::processTests()
{
  auto cConfig = pqCoreConfiguration::instance();
  if (cConfig->testScriptCount() <= 0)
  {
    return true;
  }

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  pqPersistentMainWindowStateBehavior::saveState(mainWindow);

  for (int cc = 0, max = cConfig->testScriptCount(); cc < max; ++cc)
  {
    // let the world know which test we're current running.
    cConfig->setActiveTestIndex(cc);

    if (cc > 0)
    {
      pqPersistentMainWindowStateBehavior::restoreState(mainWindow);
      pqCommandLineOptionsBehavior::resetApplication();
    }
    else if (cc == 0)
    {
      if (cConfig->testMaster())
      {
        pqCollaborationEventPlayer::waitForConnections(2);
      }
      else if (cConfig->testSlave())
      {
        pqCollaborationEventPlayer::waitForMaster(5000);
      }
    }

    const auto script = QString::fromStdString(cConfig->testScript());
    const auto baseline = QString::fromStdString(cConfig->testBaseline());
    const auto threshold = cConfig->testThreshold();
    const auto tempDir = QString::fromStdString(cConfig->testDirectory());

    // Play the test script if specified.
    pqTestUtility* testUtility = pqApplicationCore::instance()->testUtility();
    cout << "Playing: " << qPrintable(script) << endl;
    bool success = testUtility->playTests(script);
    if (success && !baseline.isEmpty())
    {
      success = pqComponentsTestUtility::CompareView(baseline, threshold, tempDir);
    }

    if (!success)
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::resetApplication()
{
  BEGIN_UNDO_EXCLUDE();
  pqServer* server = pqActiveObjects::instance().activeServer();
  server = pqApplicationCore::instance()->getObjectBuilder()->resetServer(server);
  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
}
