# cmp7trace

A diagnostic PHP 8 extension. It logs every loose comparison whose result differs
between PHP 7 and PHP 8 rules: `==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`, `switch`,
`in_array()` and `array_search()`. It was used to find such places in the forum
while the test suites ran. It is not needed to run the forum.

```sh
apt-get install php8.5-dev
phpize8.5 && ./configure --with-php-config=php-config8.5 && make
# temporarily, e.g. /etc/php/8.5/fpm/conf.d/98-cmp7trace.ini:
#   extension=/path/to/modules/cmp7trace.so
#   cmp7trace.log=/var/log/php/cmp7trace.log
#   opcache.enable=0
```

Each log line has:

* the code location: file, line and function; for `eval()`'d plugin or template
  code, also the caller and the template name
* the operator
* both operands
* the results under PHP 7 and PHP 8 rules
* the request

Each location is reported once per PHP process. Restart PHP-FPM to start over.
It slows PHP down and is meant for test systems only.
