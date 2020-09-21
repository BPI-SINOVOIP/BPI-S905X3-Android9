## 9.15\. Subscription Plans

"Subscription plans" refer to the billing relationship plan details provided
by a mobile carrier through [`SubscriptionManager.setSubscriptionPlans()`](https://developer.android.com/reference/android/telephony/SubscriptionManager.html#setSubscriptionPlans%28int, java.util.List<android.telephony.SubscriptionPlan>%29).

All device implementations:

* [C-0-1] MUST return subscription plans only to the mobile carrier app that
has originally provided them.
* [C-0-2] MUST NOT remotely back up or upload subscription plans.
* [C-0-3] MUST only allow overrides, such as [`SubscriptionManager.setSubscriptionOverrideCongested()`](https://developer.android.com/reference/android/telephony/SubscriptionManager.html#setSubscriptionOverrideCongested%28int, boolean, long%29),
from the mobile carrier app currently providing valid subscription plans.
