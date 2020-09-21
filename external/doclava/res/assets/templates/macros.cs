<?cs
# Set global vars for template features based on site and target.
?><?cs
if:dac ?><?cs
  # standard devsite warns on inline js and script tags ?><?cs
  set:enable_javascript = 1 ?><?cs
/if ?>

<?cs # A link to a package ?><?cs
def:package_link(pkg) ?>
  <a href="<?cs
          if:!pkg.federatedSite ?><?cs
            var:toroot ?><?cs
          /if ?><?cs var:pkg.link ?>"><?cs var:pkg.name ?></a><?cs
  /def ?><?cs

# A link to a type, or not if it is a primitive type
        link: whether to create a link at the top level, always creates links in
              recursive invocations.
              Overloaded version to support use of 'nav' parameter, which when true,
              will not include the generics in the class name (good for sidenav lists)
        Expects the following fields:
            .name
            .link
            .isPrimitive
            .superBounds.N.(more links)   (... super ... & ...)
            .extendsBounds.N.(more links) (... extends ... & ...)
            .typeArguments.N.(more links) (< ... >) ?><?cs
def:type_link_impl(type, link) ?><?cs call:type_link_impl2(type, link, "false") ?><?cs /def ?><?cs

def:type_link_impl2(type, link, nav) ?><?cs
  if:type.link && link=="true" ?><?cs
    if:type.federated ?><a href="<?cs var:type.link ?>"><?cs
      var:type.label ?></a><?cs
    else ?><a href="<?cs var:toroot ?><?cs var:type.link ?>"><?cs var:type.label ?></a><?cs
    /if ?><?cs
  else ?><?cs var:type.label ?><?cs
  /if ?><?cs
  if:subcount(type.extendsBounds) ?><?cs
      each:t=type.extendsBounds ?><?cs
          if:first(t) ?>&nbsp;extends&nbsp;<?cs else ?>&nbsp;&amp;&nbsp;<?cs /if ?><?cs
          call:type_link_impl(t, "true") ?><?cs
      /each ?><?cs
  /if ?><?cs
  if:subcount(type.superBounds) ?><?cs
      each:t=type.superBounds ?><?cs
          if:first(t) ?>&nbsp;super&nbsp;<?cs else ?>&nbsp;&amp;&nbsp;<?cs /if ?><?cs
          call:type_link_impl(t, "true") ?><?cs
      /each ?><?cs
  /if ?><?cs
  if:subcount(type.typeArguments) && nav=="false"
      ?>&lt;<?cs each:t=type.typeArguments ?><?cs call:type_link_impl(t, "true") ?><?cs
          if:!last(t) ?>,&nbsp;<?cs /if ?><?cs
      /each ?>&gt;<?cs
  /if ?><?cs
/def ?><?cs

def:simple_type_link(type)?><?cs
  if:type.link?><?cs
    if:type.federated ?><a href="<?cs var:type.link ?>"><?cs var:type.label ?></a><?cs
    else ?><a href="<?cs var:toroot ?><?cs var:type.link ?>"><?cs var:type.label ?></a><?cs
    /if?><?cs
  else ?><?cs var:type.label ?><?cs
  /if?><?cs
  if:subcount(type.typeArguments)?>&lt;<?cs
    each:t=type.typeArguments?><?cs
      call:type_link_impl(t, "true")?><?cs
      if:!last(t) ?>,&nbsp;<?cs
      /if ?><?cs
    /each ?>&gt;<?cs
  /if ?><?cs
/def ?><?cs

def:class_name(type) ?><?cs call:type_link_impl(type, "false") ?><?cs
/def ?><?cs
def:type_link2(type,nav) ?><?cs call:type_link_impl2(type, "true", nav) ?><?cs
/def ?><?cs
def:type_link(type) ?><?cs call:type_link2(type, "false") ?><?cs
/def ?><?cs

# a conditional link.
  if the "condition" parameter evals to true then the link is displayed
  otherwise only the text is displayed ?><?cs
def:cond_link(text, root, path, condition) ?><?cs
  if:condition ?><a href="<?cs var:root ?><?cs
  var:path ?>"><?cs /if ?><?cs var:text ?><?cs
  if:condition ?></a><?cs /if ?><?cs
/def ?><?cs

# A comma separated parameter list ?><?cs
def:parameter_list(params) ?><?cs
  each:param = params ?><?cs
      call:simple_type_link(param.type)?> <?cs
      var:param.name ?><?cs
      if: name(param)!=subcount(params)-1?>, <?cs /if ?><?cs
  /each ?><?cs
/def ?><?cs

def:doc_root_override()
  ?><?cs var:toroot ?><?cs
/def ?><?cs
# Print a list of tags (e.g. description text ?><?cs
def:tag_list(tags) ?><?cs
  each:tag = tags ?><?cs
      if:tag.name == "Text" ?><?cs var:tag.text?><?cs
      elif:tag.kind == "@more" ?><p><?cs
      elif:tag.kind == "@see" ?><code><a href="<?cs
        if:!tag.federatedSite ?><?cs
          var:toroot ?><?cs
        /if ?><?cs var:tag.href ?>"><?cs var:tag.label ?></a></code><?cs
      elif:tag.kind == "@linkplain" ?><a href="<?cs
        if:!tag.federatedSite ?><?cs
          var:toroot ?><?cs
        /if ?><?cs var:tag.href ?>"><?cs var:tag.label ?></a></a><?cs
      elif:tag.kind == "@seeHref" ?><a href="<?cs var:tag.href ?>"><?cs var:tag.label ?></a><?cs
      elif:tag.kind == "@seeJustLabel" ?><?cs var:tag.label ?><?cs
      elif:tag.kind == "@value" ?><code><a href="<?cs
        if:!tag.federatedSite ?><?cs
          var:toroot ?><?cs
        /if ?><?cs var:tag.href ?>"><?cs var:tag.text ?></a></code><?cs
      elif:tag.kind == "@code" ?><code><?cs var:tag.text ?></code><?cs
      elif:tag.kind == "@samplecode" ?><pre><?cs var:tag.text ?></pre><?cs
      elif:tag.name == "@sample" ?><pre><?cs var:tag.text ?></pre><?cs
      elif:tag.name == "@include" ?><?cs var:tag.text ?><?cs
      elif:tag.kind == "@docRoot" ?><?cs call:doc_root_override() ?><?cs
      elif:tag.kind == "@sdkCurrent" ?><?cs var:sdk.current ?><?cs
      elif:tag.kind == "@sdkCurrentVersion" ?><?cs var:sdk.version ?><?cs
      elif:tag.kind == "@sdkCurrentRelId" ?><?cs var:sdk.rel.id ?><?cs
      elif:tag.kind == "@sdkPlatformVersion" ?><?cs var:sdk.platform.version ?><?cs
      elif:tag.kind == "@sdkPlatformApiLevel" ?><?cs var:sdk.platform.apiLevel ?><?cs
      elif:tag.kind == "@sdkPlatformMajorMinor" ?><?cs var:sdk.platform.majorMinor ?><?cs
      elif:tag.kind == "@sdkPlatformReleaseDate" ?><?cs var:sdk.platform.releaseDate ?><?cs
      elif:tag.kind == "@sdkPlatformDeployableDate" ?><?cs var:sdk.platform.deployableDate ?><?cs
      elif:tag.kind == "@adtZipVersion" ?><?cs var:adt.zip.version ?><?cs
      elif:tag.kind == "@adtZipDownload" ?><?cs var:adt.zip.download ?><?cs
      elif:tag.kind == "@adtZipBytes" ?><?cs var:adt.zip.bytes ?><?cs
      elif:tag.kind == "@adtZipChecksum" ?><?cs var:adt.zip.checksum ?><?cs
      elif:tag.kind == "@inheritDoc" ?><?cs # This is the case when @inheritDoc is in something
                                              that does not inherit from anything?><?cs
      elif:tag.kind == "@attr" ?><?cs
      elif:tag.kind == "@usesMathJax" ?><?cs
        if:devsite ?><script src="/_static/js/managed/mathjax/MathJax.js?config=TeX-AMS_SVG"></script><?cs
        else ?><script src="https://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS_SVG"></script><?cs
        /if ?><?cs
      else ?>{<?cs var:tag.name?> <?cs var:tag.text ?>}<?cs
      /if ?><?cs
  /each ?><?cs
/def ?><?cs

# Print output for block tags that are not "standard" javadoc tags ?><?cs
def:block_tag_list(tags) ?><?cs
  each:tag = tags ?><?cs
      if:tag.kind == "@apiNote" ?>
        <div class="jd-tagdata">
          <h5 class="jd-tagtitle">API Note:</h5>
          <ul class="nolist"><li><?cs call:tag_list(tag.commentTags) ?></li></ul>
        </div><?cs
      /if ?><?cs
      if:tag.kind == "@implSpec" ?>
        <div class="jd-tagdata">
          <h5 class="jd-tagtitle">Implementation Requirements:</h5>
          <ul class="nolist"><li><?cs call:tag_list(tag.commentTags) ?></li></ul>
        </div><?cs
      /if ?><?cs
      if:tag.kind == "@implNote" ?>
        <div class="jd-tagdata">
          <h5 class="jd-tagtitle">Implementation Note:</h5>
          <ul class="nolist"><li><?cs call:tag_list(tag.commentTags) ?></li></ul>
        </div><?cs
      /if ?><?cs
  /each ?><?cs
/def ?><?cs

# Print output for aux tags that are not "standard" javadoc tags ?><?cs
def:aux_tag_list(tags) ?><?cs
/def ?><?cs

# Show the short-form description of something.  These come from shortDescr and deprecated ?><?cs
def:short_descr(obj) ?><?cs
  if:subcount(obj.deprecated) ?><em><?cs
    if:obj.deprecatedsince ?>
      This <?cs var:obj.kind ?> was deprecated
      in API level <?cs var:obj.deprecatedsince ?>.<?cs
    else ?>
      This <?cs var:obj.kind ?> is deprecated.<?cs
    /if ?>
    <?cs call:tag_list(obj.deprecated) ?></em><?cs
  else ?><?cs call:tag_list(obj.shortDescr) ?><?cs
  if:subcount(obj.annotationdocumentation)?><?cs
    each:annodoc=obj.annotationdocumentation ?>
    <div><?cs var:annodoc.text?></div><?cs
    /each?><?cs /if?><?cs
  /if ?><?cs
/def ?><?cs

# Show a list of annotations associated with obj
#
# pre is an HTML string to start the list.
# between is an HTML string to include between each successive element.
# post is an HTML string to end the list.
# for example, call:show_annotations_list(cl, "<p>Annotations: ", "<br />", "</p>")
# ?><?cs
def:show_annotations_list(obj, pre, between, post) ?><?cs
  each:anno = obj.showAnnotations ?><?cs
    if:first(anno) ?><?cs
      var:pre ?><?cs
    /if ?>
    @<?cs var:anno.type.label ?>(<?cs
    each:elem = anno.elementValues ?><?cs
      var:elem.name ?>&nbsp;=&nbsp;<?cs
      var:elem.value ?><?cs
      if:last(elem) == 0 ?>, <?cs
      /if ?><?cs
    /each ?>)<?cs
    if:last(anno) == 0 ?><?cs
      var:between ?><?cs
    /if ?><?cs
    if:last(anno) ?><?cs
      var:post ?><?cs
    /if ?><?cs
  /each ?><?cs
/def ?><?cs

# Show a comma-separated list of annotations associated with obj ?><?cs
def:show_simple_annotations_list(obj, pre, post) ?><?cs
  call:show_annotations_list(obj, pre, ", ", post) ?><?cs
/def ?><?cs

# Show the red box with the deprecated warning ?><?cs
def:deprecated_warning(obj) ?><?cs
  if:subcount(obj.deprecated) ?><p>
  <p class="caution"><strong><?cs
    if:obj.deprecatedsince ?>
      This <?cs var:obj.kind ?> was deprecated
      in API level <?cs var:obj.deprecatedsince ?>.<?cs
    else ?>
      This <?cs var:obj.kind ?> is deprecated.<?cs
    /if ?></strong><br/>
    <?cs call:tag_list(obj.deprecated) ?>
  </p><?cs
  /if ?><?cs
/def ?><?cs

# print the See Also: section ?><?cs
def:see_also_tags(also) ?><?cs
  if:subcount(also) ?>
  <div>
      <p><b>See also:</b></p>
      <ul class="nolist"><?cs
        each:tag=also ?><li><?cs
            if:tag.kind == "@see" ?><code><a href="<?cs
                    if:!tag.federatedSite ?><?cs
                      var:toroot ?><?cs
                    /if ?><?cs var:tag.href ?>"><?cs
                    var:tag.label ?></a></code><?cs
            elif:tag.kind == "@seeHref" ?><a href="<?cs var:tag.href ?>"><?cs var:tag.label ?></a><?cs
            elif:tag.kind == "@seeJustLabel" ?><?cs var:tag.label ?><?cs
            else ?>[ERROR: Unknown @see kind]<?cs
            /if ?></li><?cs
        /each ?>
      </ul>
  </div><?cs
  /if ?>
<?cs /def ?><?cs

# print the API Level ?><?cs
def:since_tags(obj) ?><?cs
if:reference.apilevels && obj.since ?><?cs
  if:string.slice(obj.since,0,1) > 0 ?>
    added in <?cs
      if:string.find(obj.since,'.') > -1
        ?><a href="<?cs var:toroot ?>topic/libraries/support-library/revisions.html">version<?cs
      else
        ?><a href="<?cs var:toroot ?>guide/topics/manifest/uses-sdk-element.html#ApiLevels">API level<?cs
      /if ?> <?cs
    var:obj.since ?></a><?cs
  else ?>
    <b><a href="<?cs var:toroot ?>preview/">Android <?cs var:obj.since ?> Developer Preview</a></b><?cs
  /if?><?cs
/if ?><?cs
/def ?><?cs

# print the artifact ?><?cs
def:artifact_tags(obj) ?><?cs
if:reference.artifacts && obj.artifact ?>
  belongs to Maven artifact <?cs var:obj.artifact ?></a><?cs
/if ?><?cs
/def ?><?cs

def:federated_refs(obj) ?>
  <?cs if:subcount(obj.federated) ?>
    <div>
    Also:
    <?cs each:federated=obj.federated ?>
      <a href="<?cs var:federated.url ?>"><?cs var:federated.name ?></a><?cs
      if:!last(federated) ?>,<?cs /if ?>
    <?cs /each ?>
    </div>
  <?cs /if ?>
<?cs /def ?><?cs

#
# Print the long-form description for something.
# Uses the following fields: deprecated descr seeAlso since
#
?><?cs
def:description(obj) ?><?cs
  call:deprecated_warning(obj) ?>
  <p><?cs call:tag_list(obj.descr) ?></p><?cs
  call:aux_tag_list(obj.descrAux) ?><?cs
  if:subcount(obj.annotationdocumentation)?><?cs
    each:annodoc=obj.annotationdocumentation ?>
    <div><?cs var:annodoc.text?></div><?cs
    /each?><?cs /if?><?cs
  if:subcount(obj.attrRefs) ?>
      <p><b>Related XML Attributes:</b></p>
      <ul class="nolist"><?cs
        each:attr=obj.attrRefs ?>
            <li><a href="<?cs
                    if:!attr.federatedSite ?><?cs
                      var:toroot ?><?cs
                    /if ?><?cs var:attr.href ?>"><?cs var:attr.name ?></a></li><?cs
        /each ?>
      </ul><?cs
  /if ?><?cs
  if:subcount(obj.blockTags) ?>
    <?cs call:block_tag_list(obj.blockTags) ?><?cs
  /if ?><?cs
  #
  # Print the @param tags
  #
  ?><?cs
  if:subcount(obj.paramTags) ?>
    <table class="responsive">
    <tr><th colspan=2>Parameters</th></tr><?cs
    each:param=obj.paramTags ?>
      <tr>
        <td><code><?cs
          if:param.isTypeParameter ?>&lt;<?cs
          /if ?><?cs var:param.name ?><?cs
          if:param.isTypeParameter ?>&gt;<?cs
          /if ?></code></td>
        <td width="100%">
          <code><?cs var:param.kind ?></code><?cs
          if:string.find(param.comment.0.text, "<!--") != 0
            ?>:<?cs # Do not print if param comment is an HTML comment ?><?cs
          /if ?> <?cs
          call:tag_list(param.comment) ?><?cs
          call:aux_tag_list(param.commentAux) ?></td>
      </tr><?cs
    /each ?>
    </table><?cs
  /if ?><?cs
  #
  # Print the @return value
  #
  ?><?cs
  if:subcount(obj.returns) || (subcount(method.returnType) && method.returnType.label != 'void') ?>
    <table class="responsive">
      <tr><th colspan=2>Returns</th></tr>
      <tr>
        <td><code><?cs call:type_link(method.returnType) ?></code></td>
        <td width="100%"><?cs
        if:subcount(obj.returns) ?><?cs
          call:tag_list(obj.returns) ?><?cs
        else ?><!-- no returns description in source --><?cs
        /if ?><?cs
        call:aux_tag_list(obj.returnsAux) ?></td>
      </tr>
    </table><?cs
  /if ?><?cs
  #
  # Print the throwables
  #
  ?><?cs
  if:subcount(obj.throws) ?>
      <table class="responsive">
      <tr><th colspan=2>Throws</th></tr><?cs
      each:tag=obj.throws ?>
        <tr>
          <td><code><?cs call:type_link(tag.type) ?></code></td>
          <td width="100%"><?cs call:tag_list(tag.comment) ?></td>
        </tr><?cs
      /each ?>
      </table>
  <?cs
  /if ?><?cs
  call:see_also_tags(obj.seeAlso) ?><?cs
/def ?><?cs

# A table of links to classes with descriptions, as in a package file or the nested classes ?><?cs
def:class_link_table(classes) ?><?cs
  set:count = #1 ?>
  <table class="jd-sumtable-expando"><?cs
      each:cl=classes ?>
        <tr class="<?cs if:count % #2 ?>alt-color<?cs /if ?> api apilevel-<?cs var:cl.type.since ?>" >
              <td><?cs call:type_link(cl.type) ?></td>
              <td width="100%"><?cs call:short_descr(cl) ?>&nbsp;</td>
          </tr><?cs set:count = count + #1 ?><?cs
      /each ?>
  </table><?cs
/def ?><?cs

# A list of links to classes, for use in the side navigation of classes when viewing a package (panel nav) ?><?cs
def:class_link_list(label, classes) ?><?cs
  if:subcount(classes) ?>
    <li><h2 class="hide-from-toc"><?cs var:label ?></h2>
      <ul><?cs
      each:cl=classes ?>
        <li class="api apilevel-<?cs var:cl.type.since ?>"><?cs call:type_link2(cl.type,"true") ?></li><?cs
      /each ?>
      </ul>
    </li><?cs
  /if ?><?cs
/def ?><?cs

# A list of links to classes, for use in the side navigation of classes when viewing a class (panel nav) ?><?cs
def:list(label, classes) ?><?cs
  if:subcount(classes) ?>
    <li><h2 class="hide-from-toc"><?cs var:label ?></h2>
      <ul><?cs
      each:cl=classes ?>
          <li class="<?cs if:class.name == cl.label?>selected <?cs /if ?>api apilevel-<?cs var:cl.since ?>"><?cs call:type_link2(cl,"true") ?></li><?cs
      /each ?>
      </ul>
    </li><?cs
  /if ?><?cs
/def ?><?cs

# A list of links to packages, for use in the side navigation of packages (panel nav) ?><?cs
def:package_link_list(packages) ?><?cs
  each:pkg=packages ?>
    <li class="<?cs if:(class.package.name == pkg.name) || (package.name == pkg.name)?>selected <?cs /if ?>api apilevel-<?cs var:pkg.since ?>"><?cs call:package_link(pkg) ?></li><?cs
  /each ?><?cs
/def ?>

<?cs
# An expando trigger
?><?cs
def:expando_trigger(id, default) ?>
  <a href="#" id="<?cs var:id ?>" class="jd-expando-trigger closed"<?cs
    if:enable_javascript ?>
     onclick="return toggleInherited(this, null)"<?cs
    /if ?> >
    <img id="<?cs var:id ?>-trigger" class="jd-expando-trigger-img"
         height="34"
         src="<?cs var:toroot ?>assets/images/styles/disclosure_<?cs
              if:default == 'closed' ?>down<?cs else ?>up<?cs /if ?>.png" />
  </a><?cs
/def ?>

<?cs
# An expandable list of classes
?><?cs
def:expandable_class_list(id, classes, default) ?>
  <div id="<?cs var:id ?>" class="showalways" > <?cs
    if:subcount(classes) <= #20 ?><?cs
      each:cl=classes ?><?cs
        call:type_link(cl.type) ?><?cs
        if:!last(cl)
          ?>, <?cs
        /if ?><?cs
      /each ?><?cs
    else ?><?cs
      set:leftovers = subcount(classes) - #15 ?><?cs
      loop:i = #0, #14, #1 ?><?cs
        with:cl=classes[i] ?><?cs
          call:type_link(cl.type) ?>, <?cs
        /with ?><?cs
        if:(#i == #14) ?> and <?cs var:leftovers ?> others.<?cs
        /if ?><?cs
      /loop ?><?cs
    /if ?>
  </div>
  <div id="<?cs var:id ?>-summary"<?cs
    if:default == "summary" ?>
       class="showalways"<?cs
    /if ?> >
    <?cs call:class_link_table(classes) ?>
  </div><?cs
/def ?>

<?cs include:"components.cs" ?>
