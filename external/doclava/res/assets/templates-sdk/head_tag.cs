<head>
  <title><?cs
if:devsite ?><?cs
  if:page.title ?><?cs
    var:html_strip(page.title) ?><?cs
  else ?>Android Developers<?cs
  /if ?><?cs
else ?><?cs
  if:page.title ?><?cs
    var:page.title ?> | <?cs
  /if ?>Android Developers
<?cs /if ?><?cs
# END if/else devsite ?></title><?cs
  ####### If building devsite, add some meta data needed for when generating the top nav ######### ?><?cs
if:devsite ?><?cs
  if:dac ?><?cs
    # if this build set `library.root` then set a django variable to be used by the subsequent
    # _reference-head-tags.html file for the book path (or ignored if its no longer needed)
    ?><?cs
    if:library.root ?>
      {% setvar book_path %}/reference/<?cs var:library.root ?>/_book.yaml{% endsetvar %}<?cs
    /if ?>
    {% include "_shared/_reference-head-tags.html" %}
    <meta name="body_class" value="api apilevel-<?cs var:class.since ?><?cs var:package.since ?>" /><?cs
  else ?><?cs # If NOT dac... ?>
    <meta name="hide_page_heading" value="true" />
    <meta name="book_path" value="<?cs
      if:book.path ?><?cs var:book.path ?><?cs
      else ?>/_book.yaml<?cs
      /if ?>" />
    <meta name="project_path" value="<?cs
      if:project.path ?><?cs var:project.path ?><?cs
      else ?>/_project.yaml<?cs
      /if ?>" /><?cs
  /if ?><?cs # End if/else dac ?><?cs
else ?><?cs # if NOT devsite... ?>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1.0,minimum-scale=1.0,maximum-scale=1.0,user-scalable=no" />
<meta content="IE=edge" http-equiv="X-UA-Compatible">
<link rel="shortcut icon" type="image/x-icon" href="<?cs var:toroot ?>favicon.ico" />
<link rel="alternate" href="http://developer.android.com/<?cs var:path.canonical ?>" hreflang="en" />
<link rel="alternate" href="http://developer.android.com/intl/es/<?cs var:path.canonical ?>" hreflang="es" />
<link rel="alternate" href="http://developer.android.com/intl/id/<?cs var:path.canonical ?>" hreflang="id" />
<link rel="alternate" href="http://developer.android.com/intl/ja/<?cs var:path.canonical ?>" hreflang="ja" />
<link rel="alternate" href="http://developer.android.com/intl/ko/<?cs var:path.canonical ?>" hreflang="ko" />
<link rel="alternate" href="http://developer.android.com/intl/pt-br/<?cs var:path.canonical ?>" hreflang="pt-br" />
<link rel="alternate" href="http://developer.android.com/intl/ru/<?cs var:path.canonical ?>" hreflang="ru" />
<link rel="alternate" href="http://developer.android.com/intl/vi/<?cs var:path.canonical ?>" hreflang="vi" />
<link rel="alternate" href="http://developer.android.com/intl/zh-cn/<?cs var:path.canonical ?>" hreflang="zh-cn" />
<link rel="alternate" href="http://developer.android.com/intl/zh-tw/<?cs var:path.canonical ?>" hreflang="zh-tw" />
<?cs /if ?><?cs
# END if/else !devsite ?><?cs

      if:page.metaDescription ?>
  <meta name="description" content="<?cs var:page.metaDescription ?>"><?cs
      /if ?><?cs
  if:!devsite ?>
<!-- STYLESHEETS -->
<link rel="stylesheet"
href="<?cs
if:android.whichdoc != 'online' ?>http:<?cs
/if ?>//fonts.googleapis.com/css?family=Roboto+Condensed">
<link rel="stylesheet" href="<?cs
if:android.whichdoc != 'online' ?>http:<?cs
/if ?>//fonts.googleapis.com/css?family=Roboto:light,regular,medium,thin,italic,mediumitalic,bold"
  title="roboto">
<?cs
  if:ndk ?><link rel="stylesheet" href="<?cs
  if:android.whichdoc != 'online' ?>http:<?cs
  /if ?>//fonts.googleapis.com/css?family=Roboto+Mono:400,500,700" title="roboto-mono" type="text/css"><?cs
/if ?>
<link href="<?cs var:toroot ?>assets/css/default.css?v=16" rel="stylesheet" type="text/css">

<!-- JAVASCRIPT -->
<script src="<?cs if:android.whichdoc != 'online' ?>http:<?cs /if ?>//www.google.com/jsapi" type="text/javascript"></script>
<script src="<?cs var:toroot ?>assets/js/android_3p-bundle.js" type="text/javascript"></script><?cs
  if:page.customHeadTag ?>
<?cs var:page.customHeadTag ?><?cs
  /if ?>
<script type="text/javascript">
  var toRoot = "<?cs var:toroot ?>";
  var metaTags = [<?cs var:meta.tags ?>];
  var devsite = <?cs if:devsite ?>true<?cs else ?>false<?cs /if ?>;
  var useUpdatedTemplates = <?cs if:useUpdatedTemplates ?>true<?cs else ?>false<?cs /if ?>;
</script>
<script src="<?cs var:toroot ?>assets/js/docs.js?v=17" type="text/javascript"></script>

<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-5831155-1', 'android.com');
  ga('create', 'UA-49880327-2', 'android.com', {'name': 'universal'});  // New tracker);
  ga('send', 'pageview');
  ga('universal.send', 'pageview'); // Send page view for new tracker.
</script><?cs /if ?><?cs
# END if/else !devsite ?>

<?cs if:css.path ?><?cs
#include custom stylesheet ?>
<link rel="stylesheet" href="<?cs var:css.path ?>"><?cs
/if ?>
</head>
