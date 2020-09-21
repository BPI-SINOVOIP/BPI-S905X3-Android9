<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<?cs if:page.metaDescription ?>
<meta name="Description" content="<?cs var:page.metaDescription ?>">
<?cs /if ?>
<link rel="shortcut icon" type="image/x-icon" href="<?cs var:toroot ?>favicon.ico" />
<title>
<?cs if:page.title ?>
  <?cs var:page.title ?> |
<?cs /if ?>
<?cs if:project.name ?>
  <?cs var:project.name ?>
<?cs else ?>Doclava
<?cs /if ?>
</title>
<link href="<?cs var:toroot ?>assets/doclava-developer-docs.css" rel="stylesheet" type="text/css" />
<link href="<?cs var:toroot ?>assets/customizations.css" rel="stylesheet" type="text/css" />
<script src="<?cs var:toroot ?>assets/search_autocomplete.js" type="text/javascript"></script>
<script src="<?cs var:toroot ?>assets/jquery-resizable.min.js" type="text/javascript"></script>
<script src="<?cs var:toroot ?>assets/doclava-developer-docs.js" type="text/javascript"></script>
<script src="<?cs var:toroot ?>assets/prettify.js" type="text/javascript"></script>
<script type="text/javascript">
  setToRoot("<?cs var:toroot ?>");
</script><?cs 
if:reference ?>
<script src="<?cs var:toroot ?>assets/doclava-developer-reference.js" type="text/javascript"></script>
<script src="<?cs var:toroot ?>navtree_data.js" type="text/javascript"></script><?cs 
/if ?>
<script src="<?cs var:toroot ?>assets/customizations.js" type="text/javascript"></script>
<noscript>
  <style type="text/css">
    html,body{overflow:auto;}
    #body-content{position:relative; top:0;}
    #doc-content{overflow:visible;border-left:3px solid #666;}
    #side-nav{padding:0;}
    #side-nav .toggle-list ul {display:block;}
    #resize-packages-nav{border-bottom:3px solid #666;}
  </style>
</noscript>
</head>
