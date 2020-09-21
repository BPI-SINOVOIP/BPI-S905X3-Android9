# To set up client_secrets.json (once only)

* Go to https://console.cloud.google.com/projectselector/apis/credentials/consent
 and create a new project if needed.
* Once on the consent screen, set the product name to anything and save.
* Click "Create credentials" and select "OAuth client ID"
* Under "Application type", select "Other". Click "Create".
* Click the download button for the client you just created,
 and save the resulting file at client_secrets.json

# Running unit tests
    python build_provider_pab_test.py