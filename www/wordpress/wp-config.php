<?php
/**
 * The base configuration for WordPress
 *
 * The wp-config.php creation script uses this file during the installation.
 * You don't have to use the website, you can copy this file to "wp-config.php"
 * and fill in the values.
 *
 * This file contains the following configurations:
 *
 * * Database settings
 * * Secret keys
 * * Database table prefix
 * * ABSPATH
 *
 * @link https://developer.wordpress.org/advanced-administration/wordpress/wp-config/
 *
 * @package WordPress
 */

// ** Database settings - You can get this info from your web host ** //
/** The name of the database for WordPress */
define( 'DB_NAME', 'mydatabase' );

/** Database username */
define( 'DB_USER', 'root' );

/** Database password */
define( 'DB_PASSWORD', 'rootpassword' );

/** Database hostname */
define( 'DB_HOST', 'mysql' );

/** Database charset to use in creating database tables. */
define( 'DB_CHARSET', 'utf8mb4' );

/** The database collate type. Don't change this if in doubt. */
define( 'DB_COLLATE', '' );



/**#@+
 * Authentication unique keys and salts.
 *
 * Change these to different unique phrases! You can generate these using
 * the {@link https://api.wordpress.org/secret-key/1.1/salt/ WordPress.org secret-key service}.
 *
 * You can change these at any point in time to invalidate all existing cookies.
 * This will force all users to have to log in again.
 *
 * @since 2.6.0
 */
define( 'AUTH_KEY',         '4OBJy%XWhFYI)R~=oo,x(Dh~Hf-uG$5fh9APr9!OEj3kPh,e.DyUol($^h48w_4w' );
define( 'SECURE_AUTH_KEY',  'dq_zx))wDK|pTrX!WI#LG{y3@~ yuXC9tM1! q(9uYt=ihx1#qO@#bbgC;q6kF8g' );
define( 'LOGGED_IN_KEY',    'Q%A+z>TfbeXMP==z>iQx:_}62){>dx;HRY$`<Tz P>~9AD+Uq3C~.?noM>QPRqoo' );
define( 'NONCE_KEY',        'b<)/hSpL_o*K[M$Z>)SWthc6)cv@ZD+WbN`7+i0k]Jc|mi$!OIowOMljSRL:J/7]' );
define( 'AUTH_SALT',        'tJbyUR_o6Z[(YX!,Or[d+BMqAtLF/7R`~fZb!Y?$1)i eu%oMw=o2?yZH7X-/-sT' );
define( 'SECURE_AUTH_SALT', 'xPoLMwB@s|?B3!xSz{O=`bkTqUNQS0)!MDlhc65+lvxQ=p}}kn=HKsHc2fALm#L(' );
define( 'LOGGED_IN_SALT',   'bb}.,xy+<.!?Pcce$y> RX/%C<<LOBK?&WW[HsS}Ly4z<7_[SH_I/F`i;cW=&,hr' );
define( 'NONCE_SALT',       ' :UJuYiST;ckBCWL0O+M1 u5cWQd]fyn=+tS3KVYu8?8zQL@}`{{2h@!_jl{6yGd' );

/**#@-*/

/**
 * WordPress database table prefix.
 *
 * You can have multiple installations in one database if you give each
 * a unique prefix. Only numbers, letters, and underscores please!
 *
 * At the installation time, database tables are created with the specified prefix.
 * Changing this value after WordPress is installed will make your site think
 * it has not been installed.
 *
 * @link https://developer.wordpress.org/advanced-administration/wordpress/wp-config/#table-prefix
 */
$table_prefix = 'wp_';

/**
 * For developers: WordPress debugging mode.
 *
 * Change this to true to enable the display of notices during development.
 * It is strongly recommended that plugin and theme developers use WP_DEBUG
 * in their development environments.
 *
 * For information on other constants that can be used for debugging,
 * visit the documentation.
 *
 * @link https://developer.wordpress.org/advanced-administration/debug/debug-wordpress/
 */
define( 'WP_DEBUG', true );
define( 'WP_DEBUG_LOG', true );


/* Add any custom values between this line and the "stop editing" line. */



/* That's all, stop editing! Happy publishing. */

/** Absolute path to the WordPress directory. */
if ( ! defined( 'ABSPATH' ) ) {
	define( 'ABSPATH', __DIR__ . '/' );
}

/** Sets up WordPress vars and included files. */
require_once ABSPATH . 'wp-settings.php';
