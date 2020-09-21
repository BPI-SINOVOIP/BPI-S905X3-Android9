/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.tradefed;

import com.android.tradefed.build.BootstrapBuildProviderTest;
import com.android.tradefed.build.BuildInfoTest;
import com.android.tradefed.build.DeviceBuildDescriptorTest;
import com.android.tradefed.build.DeviceBuildInfoTest;
import com.android.tradefed.build.DeviceFolderBuildInfoTest;
import com.android.tradefed.build.FileDownloadCacheTest;
import com.android.tradefed.build.KernelBuildInfoTest;
import com.android.tradefed.build.KernelDeviceBuildInfoTest;
import com.android.tradefed.build.LocalDeviceBuildProviderTest;
import com.android.tradefed.build.OtaZipfileBuildProviderTest;
import com.android.tradefed.build.SdkBuildInfoTest;
import com.android.tradefed.command.CommandFileParserTest;
import com.android.tradefed.command.CommandFileWatcherTest;
import com.android.tradefed.command.CommandOptionsTest;
import com.android.tradefed.command.CommandRunnerTest;
import com.android.tradefed.command.CommandSchedulerTest;
import com.android.tradefed.command.ConsoleTest;
import com.android.tradefed.command.VerifyTest;
import com.android.tradefed.command.remote.RemoteManagerTest;
import com.android.tradefed.command.remote.RemoteOperationTest;
import com.android.tradefed.config.ArgsOptionParserTest;
import com.android.tradefed.config.ConfigurationDefTest;
import com.android.tradefed.config.ConfigurationDescriptorTest;
import com.android.tradefed.config.ConfigurationFactoryTest;
import com.android.tradefed.config.ConfigurationTest;
import com.android.tradefed.config.ConfigurationXmlParserTest;
import com.android.tradefed.config.GlobalConfigurationTest;
import com.android.tradefed.config.OptionCopierTest;
import com.android.tradefed.config.OptionSetterTest;
import com.android.tradefed.config.OptionUpdateRuleTest;
import com.android.tradefed.config.SandboxConfigurationFactoryTest;
import com.android.tradefed.config.gcs.GCSConfigurationFactoryTest;
import com.android.tradefed.config.gcs.GCSConfigurationServerTest;
import com.android.tradefed.device.AndroidDebugBridgeWrapperTest;
import com.android.tradefed.device.BackgroundDeviceActionTest;
import com.android.tradefed.device.CpuStatsCollectorTest;
import com.android.tradefed.device.DeviceManagerTest;
import com.android.tradefed.device.DeviceSelectionOptionsTest;
import com.android.tradefed.device.DeviceStateMonitorTest;
import com.android.tradefed.device.DeviceUtilStatsMonitorTest;
import com.android.tradefed.device.DumpsysPackageReceiverTest;
import com.android.tradefed.device.FastbootHelperTest;
import com.android.tradefed.device.ManagedDeviceListTest;
import com.android.tradefed.device.ManagedTestDeviceFactoryTest;
import com.android.tradefed.device.NativeDeviceTest;
import com.android.tradefed.device.ReconnectingRecoveryTest;
import com.android.tradefed.device.RemoteAndroidDeviceTest;
import com.android.tradefed.device.TestDeviceTest;
import com.android.tradefed.device.TopHelperTest;
import com.android.tradefed.device.WaitDeviceRecoveryTest;
import com.android.tradefed.device.WifiHelperTest;
import com.android.tradefed.device.metric.AtraceCollectorTest;
import com.android.tradefed.device.metric.AtraceRunMetricCollectorTest;
import com.android.tradefed.device.metric.BaseDeviceMetricCollectorTest;
import com.android.tradefed.device.metric.BuddyInfoMetricCollectorTest;
import com.android.tradefed.device.metric.BugreportzMetricCollectorTest;
import com.android.tradefed.device.metric.DeviceMetricDataTest;
import com.android.tradefed.device.metric.DumpHeapCollectorTest;
import com.android.tradefed.device.metric.FilePullerDeviceMetricCollectorTest;
import com.android.tradefed.device.metric.FilePullerLogCollectorTest;
import com.android.tradefed.device.metric.GraphicsStatsMetricCollectorTest;
import com.android.tradefed.device.metric.IonHeapInfoMetricCollectorTest;
import com.android.tradefed.device.metric.MemInfoMetricCollectorTest;
import com.android.tradefed.device.metric.PagetypeInfoMetricCollectorTest;
import com.android.tradefed.device.metric.ProcessMaxMemoryCollectorTest;
import com.android.tradefed.device.metric.ScheduleMultipleDeviceMetricCollectorTest;
import com.android.tradefed.device.metric.ScheduledDeviceMetricCollectorTest;
import com.android.tradefed.device.metric.TemperatureCollectorTest;
import com.android.tradefed.device.metric.TraceMetricCollectorTest;
import com.android.tradefed.guice.InvocationScopeTest;
import com.android.tradefed.invoker.InvocationContextTest;
import com.android.tradefed.invoker.InvocationExecutionTest;
import com.android.tradefed.invoker.SandboxedInvocationExecutionTest;
import com.android.tradefed.invoker.ShardListenerTest;
import com.android.tradefed.invoker.ShardMasterResultForwarderTest;
import com.android.tradefed.invoker.TestInvocationMultiTest;
import com.android.tradefed.invoker.TestInvocationTest;
import com.android.tradefed.invoker.shard.ShardHelperTest;
import com.android.tradefed.invoker.shard.StrictShardHelperTest;
import com.android.tradefed.invoker.shard.TestsPoolPollerTest;
import com.android.tradefed.log.FileLoggerTest;
import com.android.tradefed.log.HistoryLoggerTest;
import com.android.tradefed.log.LogRegistryTest;
import com.android.tradefed.log.TerribleFailureEmailHandlerTest;
import com.android.tradefed.result.BugreportCollectorTest;
import com.android.tradefed.result.CollectingTestListenerTest;
import com.android.tradefed.result.ConsoleResultReporterTest;
import com.android.tradefed.result.DeviceFileReporterTest;
import com.android.tradefed.result.DeviceUnavailEmailResultReporterTest;
import com.android.tradefed.result.EmailResultReporterTest;
import com.android.tradefed.result.FailureEmailResultReporterTest;
import com.android.tradefed.result.FileMetadataCollectorTest;
import com.android.tradefed.result.FileSystemLogSaverTest;
import com.android.tradefed.result.InvocationFailureEmailResultReporterTest;
import com.android.tradefed.result.InvocationToJUnitResultForwarderTest;
import com.android.tradefed.result.JUnitToInvocationResultForwarderTest;
import com.android.tradefed.result.LegacySubprocessResultsReporterTest;
import com.android.tradefed.result.LogFileSaverTest;
import com.android.tradefed.result.LogcatCrashResultForwarderTest;
import com.android.tradefed.result.MetricsXMLResultReporterTest;
import com.android.tradefed.result.SnapshotInputStreamSourceTest;
import com.android.tradefed.result.SubprocessResultsReporterTest;
import com.android.tradefed.result.TestDescriptionTest;
import com.android.tradefed.result.TestFailureEmailResultReporterTest;
import com.android.tradefed.result.TestRunResultTest;
import com.android.tradefed.result.TestSummaryTest;
import com.android.tradefed.result.XmlResultReporterTest;
import com.android.tradefed.result.suite.FormattedGeneratorReporterTest;
import com.android.tradefed.result.suite.XmlSuiteResultFormatterTest;
import com.android.tradefed.sandbox.SandboxConfigDumpTest;
import com.android.tradefed.sandbox.SandboxConfigUtilTest;
import com.android.tradefed.sandbox.SandboxInvocationRunnerTest;
import com.android.tradefed.sandbox.TradefedSandboxTest;
import com.android.tradefed.suite.checker.ActivityStatusCheckerTest;
import com.android.tradefed.suite.checker.KeyguardStatusCheckerTest;
import com.android.tradefed.suite.checker.SystemServerFileDescriptorCheckerTest;
import com.android.tradefed.suite.checker.SystemServerStatusCheckerTest;
import com.android.tradefed.suite.checker.TimeStatusCheckerTest;
import com.android.tradefed.targetprep.AllTestAppsInstallSetupTest;
import com.android.tradefed.targetprep.AppSetupTest;
import com.android.tradefed.targetprep.BuildInfoAttributePreparerTest;
import com.android.tradefed.targetprep.DefaultTestsZipInstallerTest;
import com.android.tradefed.targetprep.DeviceFlashPreparerTest;
import com.android.tradefed.targetprep.DeviceSetupTest;
import com.android.tradefed.targetprep.DeviceStorageFillerTest;
import com.android.tradefed.targetprep.DeviceStringPusherTest;
import com.android.tradefed.targetprep.FastbootDeviceFlasherTest;
import com.android.tradefed.targetprep.FlashingResourcesParserTest;
import com.android.tradefed.targetprep.InstallAllTestZipAppsSetupTest;
import com.android.tradefed.targetprep.InstallApkSetupTest;
import com.android.tradefed.targetprep.InstrumentationPreparerTest;
import com.android.tradefed.targetprep.KernelFlashPreparerTest;
import com.android.tradefed.targetprep.PreloadedClassesPreparerTest;
import com.android.tradefed.targetprep.PushFilePreparerTest;
import com.android.tradefed.targetprep.PythonVirtualenvPreparerTest;
import com.android.tradefed.targetprep.RebootTargetPreparerTest;
import com.android.tradefed.targetprep.RestartSystemServerTargetPreparerTest;
import com.android.tradefed.targetprep.RootTargetPreparerTest;
import com.android.tradefed.targetprep.RunCommandTargetPreparerTest;
import com.android.tradefed.targetprep.RunHostCommandTargetPreparerTest;
import com.android.tradefed.targetprep.SdkAvdPreparerTest;
import com.android.tradefed.targetprep.StopServicesSetupTest;
import com.android.tradefed.targetprep.SystemUpdaterDeviceFlasherTest;
import com.android.tradefed.targetprep.TestAppInstallSetupTest;
import com.android.tradefed.targetprep.TestFilePushSetupTest;
import com.android.tradefed.targetprep.TimeSetterTargetPreparerTest;
import com.android.tradefed.targetprep.multi.MergeMultiBuildTargetPreparerTest;
import com.android.tradefed.targetprep.suite.SuiteApkInstallerTest;
import com.android.tradefed.testtype.AndroidJUnitTestTest;
import com.android.tradefed.testtype.CodeCoverageListenerTest;
import com.android.tradefed.testtype.CodeCoverageTestBaseTest;
import com.android.tradefed.testtype.DeviceBatteryLevelCheckerTest;
import com.android.tradefed.testtype.DeviceSuiteTest;
import com.android.tradefed.testtype.DeviceTestCaseTest;
import com.android.tradefed.testtype.DeviceTestSuiteTest;
import com.android.tradefed.testtype.FakeTestTest;
import com.android.tradefed.testtype.GTestListTestParserTest;
import com.android.tradefed.testtype.GTestResultParserTest;
import com.android.tradefed.testtype.GTestTest;
import com.android.tradefed.testtype.GTestXmlResultParserTest;
import com.android.tradefed.testtype.GoogleBenchmarkResultParserTest;
import com.android.tradefed.testtype.GoogleBenchmarkTestTest;
import com.android.tradefed.testtype.HostTestTest;
import com.android.tradefed.testtype.InstalledInstrumentationsTestTest;
import com.android.tradefed.testtype.InstrumentationFileTestTest;
import com.android.tradefed.testtype.InstrumentationSerialTestTest;
import com.android.tradefed.testtype.InstrumentationTestTest;
import com.android.tradefed.testtype.JacocoCodeCoverageTestTest;
import com.android.tradefed.testtype.JarHostTestTest;
import com.android.tradefed.testtype.NativeBenchmarkTestParserTest;
import com.android.tradefed.testtype.NativeBenchmarkTestTest;
import com.android.tradefed.testtype.NativeStressTestParserTest;
import com.android.tradefed.testtype.NativeStressTestTest;
import com.android.tradefed.testtype.NoisyDryRunTestTest;
import com.android.tradefed.testtype.PythonUnitTestResultParserTest;
import com.android.tradefed.testtype.PythonUnitTestRunnerTest;
import com.android.tradefed.testtype.TfTestLauncherTest;
import com.android.tradefed.testtype.VersionedTfLauncherTest;
import com.android.tradefed.testtype.host.CoverageMeasurementForwarderTest;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4TestTest;
import com.android.tradefed.testtype.junit4.DeviceParameterizedRunnerTest;
import com.android.tradefed.testtype.junit4.LongevityHostRunnerTest;
import com.android.tradefed.testtype.metricregression.DetectRegressionTest;
import com.android.tradefed.testtype.metricregression.MetricsTest;
import com.android.tradefed.testtype.python.PythonBinaryHostTestTest;
import com.android.tradefed.testtype.suite.AtestRunnerTest;
import com.android.tradefed.testtype.suite.BaseTestSuiteTest;
import com.android.tradefed.testtype.suite.GranularRetriableTestWrapperTest;
import com.android.tradefed.testtype.suite.ITestSuiteIntegrationTest;
import com.android.tradefed.testtype.suite.ITestSuiteMultiTest;
import com.android.tradefed.testtype.suite.ITestSuiteTest;
import com.android.tradefed.testtype.suite.ModuleDefinitionMultiTest;
import com.android.tradefed.testtype.suite.ModuleDefinitionTest;
import com.android.tradefed.testtype.suite.ModuleListenerTest;
import com.android.tradefed.testtype.suite.ModuleMergerTest;
import com.android.tradefed.testtype.suite.ModuleSplitterTest;
import com.android.tradefed.testtype.suite.SuiteModuleLoaderTest;
import com.android.tradefed.testtype.suite.TestFailureListenerTest;
import com.android.tradefed.testtype.suite.TestMappingSuiteRunnerTest;
import com.android.tradefed.testtype.suite.TestSuiteInfoTest;
import com.android.tradefed.testtype.suite.TfSuiteRunnerTest;
import com.android.tradefed.testtype.suite.ValidateSuiteConfigHelperTest;
import com.android.tradefed.testtype.suite.module.BaseModuleControllerTest;
import com.android.tradefed.testtype.suite.module.NativeBridgeModuleControllerTest;
import com.android.tradefed.testtype.testdefs.XmlDefsParserTest;
import com.android.tradefed.testtype.testdefs.XmlDefsTestTest;
import com.android.tradefed.util.AaptParserTest;
import com.android.tradefed.util.AbiFormatterTest;
import com.android.tradefed.util.AbiUtilsTest;
import com.android.tradefed.util.AppVersionFetcherTest;
import com.android.tradefed.util.ArrayUtilTest;
import com.android.tradefed.util.BluetoothUtilsTest;
import com.android.tradefed.util.BugreportTest;
import com.android.tradefed.util.BuildTestsZipUtilsTest;
import com.android.tradefed.util.ByteArrayListTest;
import com.android.tradefed.util.ClassPathScannerTest;
import com.android.tradefed.util.ConditionPriorityBlockingQueueTest;
import com.android.tradefed.util.ConfigCompletorTest;
import com.android.tradefed.util.DirectedGraphTest;
import com.android.tradefed.util.EmailTest;
import com.android.tradefed.util.EmmaXmlReporterParserTest;
import com.android.tradefed.util.FakeTestsZipFolderTest;
import com.android.tradefed.util.FileUtilTest;
import com.android.tradefed.util.FixedByteArrayOutputStreamTest;
import com.android.tradefed.util.HprofAllocSiteParserTest;
import com.android.tradefed.util.JUnitXmlParserTest;
import com.android.tradefed.util.KeyguardControllerStateTest;
import com.android.tradefed.util.ListInstrumentationParserTest;
import com.android.tradefed.util.LogcatUpdaterEventParserTest;
import com.android.tradefed.util.MetricsXmlParserTest;
import com.android.tradefed.util.MultiMapTest;
import com.android.tradefed.util.NullUtilTest;
import com.android.tradefed.util.PairTest;
import com.android.tradefed.util.PropertyChangerTest;
import com.android.tradefed.util.PsParserTest;
import com.android.tradefed.util.QuotationAwareTokenizerTest;
import com.android.tradefed.util.RegexTrieTest;
import com.android.tradefed.util.RunUtilTest;
import com.android.tradefed.util.SerializationUtilTest;
import com.android.tradefed.util.SimplePerfStatResultParserTest;
import com.android.tradefed.util.SimplePerfUtilTest;
import com.android.tradefed.util.SimpleStatsTest;
import com.android.tradefed.util.SizeLimitedOutputStreamTest;
import com.android.tradefed.util.StreamUtilTest;
import com.android.tradefed.util.StringEscapeUtilsTest;
import com.android.tradefed.util.SubprocessTestResultsParserTest;
import com.android.tradefed.util.TableBuilderTest;
import com.android.tradefed.util.TableFormatterTest;
import com.android.tradefed.util.TarUtilTest;
import com.android.tradefed.util.TestLoaderTest;
import com.android.tradefed.util.TestMappingTest;
import com.android.tradefed.util.TimeUtilTest;
import com.android.tradefed.util.TimeValTest;
import com.android.tradefed.util.ZipUtil2Test;
import com.android.tradefed.util.ZipUtilTest;
import com.android.tradefed.util.hostmetric.AbstractHostMonitorTest;
import com.android.tradefed.util.hostmetric.HeapHostMonitorTest;
import com.android.tradefed.util.keystore.JSONFileKeyStoreClientTest;
import com.android.tradefed.util.keystore.JSONFileKeyStoreFactoryTest;
import com.android.tradefed.util.net.HttpHelperTest;
import com.android.tradefed.util.net.HttpMultipartPostTest;
import com.android.tradefed.util.net.XmlRpcHelperTest;
import com.android.tradefed.util.proto.TfMetricProtoUtilTest;
import com.android.tradefed.util.sl4a.Sl4aClientTest;
import com.android.tradefed.util.sl4a.Sl4aEventDispatcherTest;
import com.android.tradefed.util.xml.AndroidManifestWriterTest;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

/**
 * A test suite for all Trade Federation unit tests running under Junit4.
 *
 * <p>All tests listed here should be self-contained, and should not require any external
 * dependencies.
 */
@RunWith(Suite.class)
@SuiteClasses({

    // build
    BootstrapBuildProviderTest.class,
    BuildInfoTest.class,
    DeviceBuildInfoTest.class,
    DeviceBuildDescriptorTest.class,
    DeviceFolderBuildInfoTest.class,
    FileDownloadCacheTest.class,
    KernelBuildInfoTest.class,
    KernelDeviceBuildInfoTest.class,
    LocalDeviceBuildProviderTest.class,
    OtaZipfileBuildProviderTest.class,
    SdkBuildInfoTest.class,

    // command
    CommandFileParserTest.class,
    CommandFileWatcherTest.class,
    CommandOptionsTest.class,
    CommandRunnerTest.class,
    CommandSchedulerTest.class,
    ConsoleTest.class,
    VerifyTest.class,

    // command.remote
    RemoteManagerTest.class,
    RemoteOperationTest.class,

    // config
    ArgsOptionParserTest.class,
    ConfigurationDefTest.class,
    ConfigurationDescriptorTest.class,
    ConfigurationFactoryTest.class,
    ConfigurationTest.class,
    ConfigurationXmlParserTest.class,
    GlobalConfigurationTest.class,
    OptionCopierTest.class,
    OptionSetterTest.class,
    OptionUpdateRuleTest.class,
    SandboxConfigurationFactoryTest.class,

    // config.gcs
    GCSConfigurationServerTest.class,
    GCSConfigurationFactoryTest.class,

    // device
    AndroidDebugBridgeWrapperTest.class,
    BackgroundDeviceActionTest.class,
    CpuStatsCollectorTest.class,
    DeviceManagerTest.class,
    DeviceSelectionOptionsTest.class,
    DeviceStateMonitorTest.class,
    DeviceUtilStatsMonitorTest.class,
    DumpsysPackageReceiverTest.class,
    FastbootHelperTest.class,
    ManagedDeviceListTest.class,
    ManagedTestDeviceFactoryTest.class,
    NativeDeviceTest.class,
    ReconnectingRecoveryTest.class,
    RemoteAndroidDeviceTest.class,
    PropertyChangerTest.class,
    TestDeviceTest.class,
    TopHelperTest.class,
    WaitDeviceRecoveryTest.class,
    WifiHelperTest.class,

    // device.metric
    AtraceCollectorTest.class,
    AtraceRunMetricCollectorTest.class,
    BaseDeviceMetricCollectorTest.class,
    BuddyInfoMetricCollectorTest.class,
    BugreportzMetricCollectorTest.class,
    DeviceMetricDataTest.class,
    DumpHeapCollectorTest.class,
    FilePullerDeviceMetricCollectorTest.class,
    FilePullerLogCollectorTest.class,
    GraphicsStatsMetricCollectorTest.class,
    IonHeapInfoMetricCollectorTest.class,
    MemInfoMetricCollectorTest.class,
    PagetypeInfoMetricCollectorTest.class,
    ProcessMaxMemoryCollectorTest.class,
    ScheduledDeviceMetricCollectorTest.class,
    ScheduleMultipleDeviceMetricCollectorTest.class,
    TemperatureCollectorTest.class,
    TraceMetricCollectorTest.class,

    // Guice
    InvocationScopeTest.class,

    // invoker
    InvocationContextTest.class,
    InvocationExecutionTest.class,
    SandboxedInvocationExecutionTest.class,
    ShardListenerTest.class,
    ShardMasterResultForwarderTest.class,
    TestInvocationMultiTest.class,
    TestInvocationTest.class,

    // invoker.shard
    ShardHelperTest.class,
    StrictShardHelperTest.class,
    TestsPoolPollerTest.class,

    // log
    FileLoggerTest.class,
    HistoryLoggerTest.class,
    LogRegistryTest.class,
    TerribleFailureEmailHandlerTest.class,

    // result
    BugreportCollectorTest.class,
    CollectingTestListenerTest.class,
    ConsoleResultReporterTest.class,
    DeviceFileReporterTest.class,
    DeviceUnavailEmailResultReporterTest.class,
    EmailResultReporterTest.class,
    EmmaXmlReporterParserTest.class,
    FailureEmailResultReporterTest.class,
    FileMetadataCollectorTest.class,
    FileSystemLogSaverTest.class,
    InvocationFailureEmailResultReporterTest.class,
    InvocationToJUnitResultForwarderTest.class,
    JUnitToInvocationResultForwarderTest.class,
    LogcatCrashResultForwarderTest.class,
    LogFileSaverTest.class,
    MetricsXMLResultReporterTest.class,
    SnapshotInputStreamSourceTest.class,
    SubprocessResultsReporterTest.class,
    TestDescriptionTest.class,
    TestFailureEmailResultReporterTest.class,
    TestRunResultTest.class,
    TestSummaryTest.class,
    XmlResultReporterTest.class,

    // result.suite
    FormattedGeneratorReporterTest.class,
    XmlSuiteResultFormatterTest.class,

    // targetprep
    AllTestAppsInstallSetupTest.class,
    AppSetupTest.class,
    BuildInfoAttributePreparerTest.class,
    DefaultTestsZipInstallerTest.class,
    DeviceFlashPreparerTest.class,
    DeviceSetupTest.class,
    DeviceStorageFillerTest.class,
    DeviceStringPusherTest.class,
    FastbootDeviceFlasherTest.class,
    FlashingResourcesParserTest.class,
    InstallAllTestZipAppsSetupTest.class,
    InstallApkSetupTest.class,
    InstrumentationPreparerTest.class,
    KernelFlashPreparerTest.class,
    PreloadedClassesPreparerTest.class,
    PushFilePreparerTest.class,
    PythonVirtualenvPreparerTest.class,
    RebootTargetPreparerTest.class,
    RestartSystemServerTargetPreparerTest.class,
    RootTargetPreparerTest.class,
    RunCommandTargetPreparerTest.class,
    RunHostCommandTargetPreparerTest.class,
    SdkAvdPreparerTest.class,
    StopServicesSetupTest.class,
    SystemUpdaterDeviceFlasherTest.class,
    TestAppInstallSetupTest.class,
    TestFilePushSetupTest.class,
    TimeSetterTargetPreparerTest.class,

    // targetprep.multi
    MergeMultiBuildTargetPreparerTest.class,

    // targetprep.suite
    SuiteApkInstallerTest.class,

    // sandbox
    SandboxConfigDumpTest.class,
    SandboxConfigUtilTest.class,
    SandboxInvocationRunnerTest.class,
    TradefedSandboxTest.class,

    // suite/checker
    ActivityStatusCheckerTest.class,
    KeyguardStatusCheckerTest.class,
    SystemServerFileDescriptorCheckerTest.class,
    SystemServerStatusCheckerTest.class,
    TimeStatusCheckerTest.class,

    // testtype
    AndroidJUnitTestTest.class,
    CodeCoverageListenerTest.class,
    CodeCoverageTestBaseTest.class,
    CoverageMeasurementForwarderTest.class,
    DeviceBatteryLevelCheckerTest.class,
    DeviceSuiteTest.class,
    DeviceTestCaseTest.class,
    DeviceTestSuiteTest.class,
    FakeTestTest.class,
    GoogleBenchmarkResultParserTest.class,
    GoogleBenchmarkTestTest.class,
    GTestListTestParserTest.class,
    GTestResultParserTest.class,
    GTestTest.class,
    GTestXmlResultParserTest.class,
    HostTestTest.class,
    InstalledInstrumentationsTestTest.class,
    InstrumentationSerialTestTest.class,
    InstrumentationFileTestTest.class,
    InstrumentationTestTest.class,
    JacocoCodeCoverageTestTest.class,
    JarHostTestTest.class,
    NativeBenchmarkTestParserTest.class,
    NativeBenchmarkTestTest.class,
    NativeStressTestParserTest.class,
    NativeStressTestTest.class,
    NoisyDryRunTestTest.class,
    PythonUnitTestResultParserTest.class,
    PythonUnitTestRunnerTest.class,
    TfTestLauncherTest.class,
    VersionedTfLauncherTest.class,

    // testtype/junit4
    BaseHostJUnit4TestTest.class,
    DeviceParameterizedRunnerTest.class,
    LongevityHostRunnerTest.class,

    // testtype/metricregression
    DetectRegressionTest.class,
    MetricsTest.class,

    // testtype/python
    PythonBinaryHostTestTest.class,

    // testtype/suite
    AtestRunnerTest.class,
    BaseTestSuiteTest.class,
    GranularRetriableTestWrapperTest.class,
    ITestSuiteIntegrationTest.class,
    ITestSuiteMultiTest.class,
    ITestSuiteTest.class,
    ModuleDefinitionMultiTest.class,
    ModuleDefinitionTest.class,
    ModuleListenerTest.class,
    ModuleMergerTest.class,
    ModuleSplitterTest.class,
    SuiteModuleLoaderTest.class,
    TestFailureListenerTest.class,
    TestMappingSuiteRunnerTest.class,
    TestSuiteInfoTest.class,
    TfSuiteRunnerTest.class,
    ValidateSuiteConfigHelperTest.class,

    // testtype/suite/module
    BaseModuleControllerTest.class,
    NativeBridgeModuleControllerTest.class,

    // testtype/testdefs
    XmlDefsParserTest.class,
    XmlDefsTestTest.class,

    // util
    AaptParserTest.class,
    AbiFormatterTest.class,
    AbiUtilsTest.class,
    AppVersionFetcherTest.class,
    ArrayUtilTest.class,
    BluetoothUtilsTest.class,
    BugreportTest.class,
    BuildTestsZipUtilsTest.class,
    ByteArrayListTest.class,
    ClassPathScannerTest.class,
    ConditionPriorityBlockingQueueTest.class,
    ConfigCompletorTest.class,
    DirectedGraphTest.class,
    EmailTest.class,
    FakeTestsZipFolderTest.class,
    FileUtilTest.class,
    FixedByteArrayOutputStreamTest.class,
    HprofAllocSiteParserTest.class,
    JUnitXmlParserTest.class,
    KeyguardControllerStateTest.class,
    LegacySubprocessResultsReporterTest.class,
    ListInstrumentationParserTest.class,
    LogcatUpdaterEventParserTest.class,
    MetricsXmlParserTest.class,
    MultiMapTest.class,
    NullUtilTest.class,
    PairTest.class,
    PsParserTest.class,
    QuotationAwareTokenizerTest.class,
    RegexTrieTest.class,
    RunUtilTest.class,
    SerializationUtilTest.class,
    SimplePerfStatResultParserTest.class,
    SimplePerfUtilTest.class,
    SimpleStatsTest.class,
    SizeLimitedOutputStreamTest.class,
    StreamUtilTest.class,
    StringEscapeUtilsTest.class,
    SubprocessTestResultsParserTest.class,
    TableBuilderTest.class,
    TableFormatterTest.class,
    TarUtilTest.class,
    TestLoaderTest.class,
    TestMappingTest.class,
    TimeUtilTest.class,
    TimeValTest.class,
    ZipUtilTest.class,
    ZipUtil2Test.class,

    // util/hostmetric
    AbstractHostMonitorTest.class,
    HeapHostMonitorTest.class,

    // util/net
    HttpHelperTest.class,
    HttpMultipartPostTest.class,
    XmlRpcHelperTest.class,

    // util/keystore
    JSONFileKeyStoreClientTest.class,
    JSONFileKeyStoreFactoryTest.class,

    // util/proto
    TfMetricProtoUtilTest.class,

    // util/sl4a
    Sl4aClientTest.class,
    Sl4aEventDispatcherTest.class,

    // util/xml
    AndroidManifestWriterTest.class,
})
public class UnitTests {
    // empty of purpose
}
