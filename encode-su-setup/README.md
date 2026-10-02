# encode.su forum: local mirror setup

Restores the encode.su vBulletin forum from two archives and serves it at
**http://encode.su** on an Ubuntu 24.04 machine. `encode.su` resolves to
127.0.0.1 through `/etc/hosts`.

The forum runs on **PHP 8.5**. vBulletin 4 does not support PHP 8, so `php85/` holds
a port:
* a patch for the forum scripts
* a fixer for the PHP code stored in the database

Both also work on PHP 5.6. See [php85/README.md](php85/README.md) for what was
changed and tested, and how to apply it to the live forum.

| Archive | Contents |
|---|---|
| `https://nishi.dreamhosters.com/20210323.tar.xz` (2.3 GB) | forum files: vBulletin **4.2.6 by vBS** scripts, styles, attachments in `u/` (snapshot of 2021-03-23) |
| `https://nishi.dreamhosters.com/20251008.sql.xz` (106 MB) | `mysqldump` of `vbulletin_encode` (MySQL 8.0.28, 259 `mst_` tables, data up to 2025-10-08) |

Restored data: 8 forums, 4,002 threads, 84,814 posts, 3,382 users.

## Usage

```sh
sudo ./setup.sh        # everything: download, install, configure, import, smoke test
sudo ./start.sh        # after a container/machine restart (there is no systemd here)
./smoke_test.sh        # index -> subforum -> thread -> post, legacy URLs, attachment, CSS, PHP handler
```

`setup.sh` is idempotent, so re-running it skips finished steps. Environment variables:

| Variable | Effect |
|---|---|
| `REIMPORT=1` | drop the database and import the dump again |
| `WORKDIR` | where the archives are downloaded (default `/home/user/dl`) |
| `WEBROOT` | where the files are extracted (default `/var/www`, giving `/var/www/encode.su`) |
| `PHP_VERSION=5.6` | run the forum on PHP 5.6, the version the archive was written for (default `8.5`) |

## Stack

* **Apache 2.4.58** with `mpm_event`, `mod_proxy_fcgi`, `mod_rewrite` and the vhost
  `conf/apache-encode.su.conf`.
  * `.php` files go to PHP-FPM through a unix socket.
  * `AllowOverride All` lets the forum's own `.htaccess` handle friendly URLs
    (`/forums/2-Data-Compression`, `/threads/3422-...`).
* **PHP 8.5** (`php8.5-fpm`) from `ppa:ondrej/php`. The PPA key is checked against
  its fingerprint.
  * `conf/php-99-encode-su.ini` sets the timezone, memory and upload limits.
  * Errors are logged to `/var/log/php/php8.5-encode.log` and not displayed.
  * `php` on the command line is the same version.
  * `PHP_VERSION=5.6` installs `php5.6-fpm` instead.
* **MySQL 8.0.46** from Ubuntu, with `conf/mysql-zz-encode-su.cnf`. Two settings
  there are needed only for PHP 5.6 to talk to MySQL 8. PHP 8.5 works either way.
  * `default_authentication_plugin = mysql_native_password`, because PHP 5.6
    mysqlnd cannot do `caching_sha2_password`.
  * `character-set-server = utf8mb3`, because PHP 5.6 mysqlnd rejects the MySQL 8
    default collation id 255 ("Server sent charset unknown to the client").

  The startup log shows deprecation warnings for both settings. They are expected.

## Matching `includes/config.php`

`config.php` is used unchanged:

* Database `vbulletin_encode`, table prefix `mst_`, driver `mysqli`, charset `utf8`, persistent connections.
* User `encode_encode`. `vb_db_sql.php` reads the password from `config.php` and creates the
  account for `localhost`, `127.0.0.1` and `::1` with `mysql_native_password`, so the password never
  appears in this repository.
* Server name `mysql.ctxmodel.net`. That is the host `config.php` actually names, not
  `robleto.iad1-mysql-e2-12a.dreamhost.com`, which is not referenced by any file in the archive.
  Both names are mapped to 127.0.0.1 and ::1, along with `encode.su` and `www.encode.su`.

## Changes made to the restored data

Production runs on `https://encode.su`. This mirror is plain HTTP, so links pointing at
the HTTPS site would leave the mirror. Only these changes are made, plus the PHP 8 port
(`php85/`: the script patch and `vb_php8_db_fix.php` for plugins and compiled templates):

1. Settings `bburl` and `vbforum_url` are changed from `https://encode.su` to `http://encode.su`
   with `vb_set_option.php`, which runs vBulletin's own `build_options()` just like saving the settings in the Admin CP.
   `vbforum_url` is what forum/thread/member links are built from.
2. Templates `headinclude` and `newattachment` of the custom styles hardcoded
   `https://encode.su/` (for `AJAXBASEURL` and the upload form action). They now use
   `http://encode.su/`; see `local_mirror_fixes.sql`.
3. All styles are rebuilt with `vb_rebuild_styles.php`, which is the Admin CP "Rebuild all styles" action.
   With "store CSS as files" on, vBulletin writes image URLs into
   `clientscript/vbulletin_css/` as absolute `bburl` URLs. Apart from `https` becoming `http`, the regenerated
   CSS is byte-identical to the archived files.

To go back to HTTPS URLs, run `php vb_set_option.php /var/www/encode.su bburl https://encode.su`
(and the same for `vbforum_url`), then rebuild the styles.

## Known gaps (they come from the archives, not from the setup)

* **Attachments after 2021-03-22 are missing.** The file archive predates the database by 4.5 years.
  5,820 of the 8,689 attachment files are present, and every one of them matches its database size.
  The 2,868 files uploaded after 2021-03-22 are not in the archive, and one file from 2010 is also missing.
  For those, `attachment.php` returns vBulletin's placeholder image.
* **`clientscript/post_thanks.js` is not in the archive.** It belongs to the Post Thank You Hack 7.84 add-on.
  A copy was supplied separately and `setup.sh` installs it from `files/post_thanks.js`.
* **Guest permissions are as in production.** Member profiles and the member list ask guests to log in,
  and the "Non-public" forum is hidden from guests. The vBulletin archive (`/archive/`) is disabled
  in the settings and redirects to `forum.php`.
* **External page resources.** Google Analytics and the Yahoo YUI CDN are referenced by the templates. When the CDN
  is unreachable, the page falls back to the bundled YUI.
* **Email.** The production SMTP settings (`smtp.dreamhost.com:587`) are kept. Scheduled tasks run
  from page views (`cron.php`), just as on the live site. In this container, outbound SMTP is blocked and
  there is no `sendmail`, so nothing can be sent. On a machine with internet access, consider
  `php vb_set_option.php /var/www/encode.su enableemail 0` so that the mirror cannot email real users.
* **Blog cron tasks.** `blog_cleanup` and `blog_pending` fail on PHP 5.6 and 8.5 alike: the
  blog product's bitfields and part of its schema are missing from the data.

## Files

| File | Purpose |
|---|---|
| `setup.sh` | full, idempotent setup |
| `start.sh` | re-add hosts entries and start MySQL, PHP-FPM and Apache after a restart |
| `smoke_test.sh` | HTTP checks of the main page types |
| `conf/` | Apache vhost, MySQL and PHP config as installed |
| `vb_db_sql.php` | prints `CREATE DATABASE/USER` SQL from `config.php` |
| `vb_set_option.php` | changes a vBulletin setting and rebuilds the options datastore |
| `vb_rebuild_styles.php` | rebuilds all styles and the CSS files |
| `local_mirror_fixes.sql` | the two template URL fixes |
| `files/post_thanks.js` | Post Thank You Hack 7.84 script missing from the archive |
| `php85/vbulletin-php85.patch` | PHP 8 port of the forum scripts (186 files) |
| `php85/vb_php8_db_fix.php` | PHP 8 port of the PHP code stored in the database (plugins, templates) |
| `php85/php8_barewords.php`, `php85/php_constants.txt` | bareword quoting used by `vb_php8_db_fix.php` |
| `php85/php85fix.sh`, `php85/php85undo.sh` | apply the PHP 8 port to a forum in place, with a backup of the files and the database, and undo it |
| `php85/README.md`, `php85/CHANGES.md` | what the port changes, how it was tested, how to apply it |
| `php85/cmp7trace/` | diagnostic PHP extension used to test the port (not needed to run the forum) |
