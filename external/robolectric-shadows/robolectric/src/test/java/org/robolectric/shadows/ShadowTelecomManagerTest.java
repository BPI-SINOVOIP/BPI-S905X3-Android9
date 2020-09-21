package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.LOLLIPOP;
import static android.os.Build.VERSION_CODES.LOLLIPOP_MR1;
import static android.os.Build.VERSION_CODES.M;
import static org.assertj.core.api.Assertions.assertThat;
import static org.robolectric.Shadows.shadowOf;

import android.content.ComponentName;
import android.content.Context;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(minSdk = LOLLIPOP)
public class ShadowTelecomManagerTest {

  private TelecomManager telecomService;

  @Before
  public void setUp() {
    telecomService = (TelecomManager) RuntimeEnvironment.application.getSystemService(Context.TELECOM_SERVICE);
  }

  @Test
  public void getSimCallManager() {
    PhoneAccountHandle handle = createHandle("id");

    shadowOf(telecomService).setSimCallManager(handle);

    assertThat(telecomService.getConnectionManager().getId()).isEqualTo("id");
  }

  @Test
  public void registerAndUnRegister() {
    assertThat(shadowOf(telecomService).getAllPhoneAccountsCount()).isEqualTo(0);
    assertThat(shadowOf(telecomService).getAllPhoneAccounts()).hasSize(0);

    PhoneAccountHandle handler = createHandle("id");
    PhoneAccount phoneAccount = PhoneAccount.builder(handler, "main_account").build();
    telecomService.registerPhoneAccount(phoneAccount);

    assertThat(shadowOf(telecomService).getAllPhoneAccountsCount()).isEqualTo(1);
    assertThat(shadowOf(telecomService).getAllPhoneAccounts()).hasSize(1);
    assertThat(telecomService.getAllPhoneAccountHandles()).hasSize(1);
    assertThat(telecomService.getAllPhoneAccountHandles()).contains(handler);
    assertThat(telecomService.getPhoneAccount(handler).getLabel()).isEqualTo(phoneAccount.getLabel());

    telecomService.unregisterPhoneAccount(handler);

    assertThat(shadowOf(telecomService).getAllPhoneAccountsCount()).isEqualTo(0);
    assertThat(shadowOf(telecomService).getAllPhoneAccounts()).hasSize(0);
    assertThat(telecomService.getAllPhoneAccountHandles()).hasSize(0);
  }

  @Test
  public void clearAccounts() {
    PhoneAccountHandle anotherPackageHandle = createHandle("some.other.package", "id");
    telecomService.registerPhoneAccount(PhoneAccount.builder(anotherPackageHandle, "another_package")
        .build());
  }

  @Test
  @Config(minSdk = LOLLIPOP_MR1)
  public void clearAccountsForPackage() {
    PhoneAccountHandle accountHandle1 = createHandle("a.package", "id1");
    telecomService.registerPhoneAccount(PhoneAccount.builder(accountHandle1, "another_package")
        .build());

    PhoneAccountHandle accountHandle2 = createHandle("some.other.package", "id2");
    telecomService.registerPhoneAccount(PhoneAccount.builder(accountHandle2, "another_package")
        .build());

    telecomService.clearAccountsForPackage(accountHandle1.getComponentName().getPackageName());

    assertThat(telecomService.getPhoneAccount(accountHandle1)).isNull();
    assertThat(telecomService.getPhoneAccount(accountHandle2)).isNotNull();
  }

  @Test
  public void getPhoneAccountsSupportingScheme() {
    PhoneAccountHandle handleMatchingScheme = createHandle("id1");
    telecomService.registerPhoneAccount(PhoneAccount.builder(handleMatchingScheme, "some_scheme")
        .addSupportedUriScheme("some_scheme")
        .build());
    PhoneAccountHandle handleNotMatchingScheme = createHandle("id2");
    telecomService.registerPhoneAccount(PhoneAccount.builder(handleNotMatchingScheme, "another_scheme")
        .addSupportedUriScheme("another_scheme")
        .build());

    List<PhoneAccountHandle> actual = telecomService.getPhoneAccountsSupportingScheme("some_scheme");

    assertThat(actual).contains(handleMatchingScheme);
    assertThat(actual).doesNotContain(handleNotMatchingScheme);
  }

  @Test
  @Config(minSdk = M)
  public void getCallCapablePhoneAccounts() {
    PhoneAccountHandle callCapableHandle = createHandle("id1");
    telecomService.registerPhoneAccount(PhoneAccount.builder(callCapableHandle, "enabled")
        .setIsEnabled(true)
        .build());
    PhoneAccountHandle notCallCapableHandler = createHandle("id2");
    telecomService.registerPhoneAccount(PhoneAccount.builder(notCallCapableHandler, "disabled")
        .setIsEnabled(false)
        .build());

    List<PhoneAccountHandle> callCapablePhoneAccounts = telecomService.getCallCapablePhoneAccounts();
    assertThat(callCapablePhoneAccounts).contains(callCapableHandle);
    assertThat(callCapablePhoneAccounts).doesNotContain(notCallCapableHandler);
  }

  @Test
  @Config(minSdk = LOLLIPOP_MR1)
  public void getPhoneAccountsForPackage() {
    PhoneAccountHandle handleInThisApplicationsPackage = createHandle("id1");
    telecomService.registerPhoneAccount(PhoneAccount.builder(handleInThisApplicationsPackage, "this_package")
        .build());

    PhoneAccountHandle anotherPackageHandle = createHandle("some.other.package", "id2");
    telecomService.registerPhoneAccount(PhoneAccount.builder(anotherPackageHandle, "another_package")
        .build());

    List<PhoneAccountHandle> phoneAccountsForPackage = telecomService.getPhoneAccountsForPackage();

    assertThat(phoneAccountsForPackage).contains(handleInThisApplicationsPackage);
    assertThat(phoneAccountsForPackage).doesNotContain(anotherPackageHandle);
  }

  @Test
  public void testAddNewIncomingCall() {
    telecomService.addNewIncomingCall(createHandle("id"), null);

    assertThat(shadowOf(telecomService).getAllIncomingCalls()).hasSize(1);
  }

  @Test
  public void testAddUnknownCall() {
    telecomService.addNewUnknownCall(createHandle("id"), null);

    assertThat(shadowOf(telecomService).getAllUnknownCalls()).hasSize(1);
  }

  @Test
  @Config(minSdk = M)
  public void setDefaultDialerPackage() {
    shadowOf(telecomService).setDefaultDialer("some.package");
    assertThat(telecomService.getDefaultDialerPackage()).isEqualTo("some.package");
  }

  private static PhoneAccountHandle createHandle(String id) {
    return createHandle(RuntimeEnvironment.application.getPackageName(), id);
  }

  private static PhoneAccountHandle createHandle(String packageName, String id) {
    return new PhoneAccountHandle(new ComponentName(packageName, "component_class_name"), id);
  }
}
