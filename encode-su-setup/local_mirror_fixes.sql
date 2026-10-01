-- Local-mirror adjustments that are plain SQL (settings that live in the
-- 'options' datastore are changed with vb_set_option.php instead).
--
-- The production style hardcodes the HTTPS site URL in two templates:
--   headinclude:   var AJAXBASEURL = "https://encode.su/";   (stock vB4: {vb:raw ajaxbaseurl})
--   newattachment: <form action="https://encode.su/newattachment.php?do=manageattach" ...>
-- Served over plain HTTP these break AJAX features and attachment uploads, so
-- point them at http://encode.su/. Both the compiled (template) and source
-- (template_un) columns are changed; the replacement is plain text, no template syntax.
UPDATE mst_template
   SET template    = REPLACE(template,    'https://encode.su/', 'http://encode.su/'),
       template_un = REPLACE(template_un, 'https://encode.su/', 'http://encode.su/')
 WHERE title IN ('headinclude', 'newattachment')
   AND styleid > 0
   AND (template LIKE '%https://encode.su/%' OR template_un LIKE '%https://encode.su/%');
