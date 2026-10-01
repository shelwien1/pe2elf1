<?php
// PHP 8 throws Error for undefined constants; PHP 7 and older used the constant's name as a
// string (with a notice/warning). Old code relies on that everywhere ($a[key], THIS_SCRIPT ==
// member, is_browser(mozilla) ...). These helpers find such barewords with the tokenizer and
// quote them, which is exactly what PHP 7 did at run time.

/**
* Names of constants defined in PHP code: define('NAME', ...) and const NAME = ...
*/
function php8_defined_constant_names($code)
{
	$names = array();
	if (preg_match_all('#\bdefine\s*\(\s*([\'"])([A-Za-z_][A-Za-z0-9_]*)\1#', $code, $m))
	{
		$names = array_merge($names, $m[2]);
	}
	if (preg_match_all('#\bconst\s+([A-Za-z_][A-Za-z0-9_]*)\s*=#', $code, $m))
	{
		$names = array_merge($names, $m[1]);
	}
	return $names;
}

/**
* Quote identifiers that PHP would treat as (undefined) constants.
*
* @param	string	PHP code starting with <?php
* @param	array	Known constant names as keys (in addition to get_defined_constants())
* @param	array	Receives the quoted names
* @return	string	Fixed code (unchanged if nothing was found)
*/
function php8_quote_undefined_constants($code, $known, &$quoted)
{
	$quoted = array();
	$all = token_get_all($code);

	// Indexes of tokens that are code: not text inside "..." / heredocs, but including
	// the {$...} parts of such strings (complex interpolation is code).
	$code_idx = array();
	$in_string = false;
	$stack = array();
	foreach ($all AS $i => $x)
	{
		$type = is_array($x) ? $x[0] : $x;
		if (!$stack AND ($type === '"' OR $type === T_START_HEREDOC OR $type === T_END_HEREDOC))
		{
			$in_string = ($type === '"') ? !$in_string : ($type === T_START_HEREDOC);
			continue;
		}
		if ($in_string AND ($type === T_CURLY_OPEN OR $type === T_DOLLAR_OPEN_CURLY_BRACES))
		{
			$stack[] = true;
			continue;
		}
		if ($in_string AND $stack)
		{
			if ($type === '{')
			{
				$stack[] = false;
			}
			if ($type === '}')
			{
				array_pop($stack);
				if (!$stack)
				{
					continue;
				}
			}
		}
		if ($in_string AND !$stack)
		{
			continue;
		}
		if (!is_array($x) OR !in_array($x[0], array(T_WHITESPACE, T_COMMENT, T_DOC_COMMENT)))
		{
			$code_idx[] = $i;
		}
	}

	$skip_prev = array(T_OBJECT_OPERATOR, T_DOUBLE_COLON, T_FUNCTION, T_CONST, T_CLASS, T_NEW,
		T_INSTANCEOF, T_EXTENDS, T_IMPLEMENTS, T_NAMESPACE, T_USE, T_GOTO, T_INTERFACE, T_TRAIT,
		T_AS, T_INSTEADOF, T_CATCH);
	if (defined('T_NULLSAFE_OBJECT_OPERATOR'))
	{
		$skip_prev[] = T_NULLSAFE_OBJECT_OPERATOR;
	}
	$keywords = array('true', 'false', 'null', 'self', 'parent', 'static');

	$n = count($code_idx);
	$in_class_header = false;
	for ($k = 0; $k < $n; $k++)
	{
		$i = $code_idx[$k];
		$x = $all[$i];
		if (!is_array($x) OR $x[0] !== T_STRING)
		{
			if (is_array($x) AND in_array($x[0], array(T_EXTENDS, T_IMPLEMENTS)))
			{
				$in_class_header = true;
			}
			if ($x === '{')
			{
				$in_class_header = false;
			}
			continue;
		}
		$name = $x[1];
		$prev = $k > 0 ? $all[$code_idx[$k - 1]] : null;
		$next = $k + 1 < $n ? $all[$code_idx[$k + 1]] : null;

		if ($in_class_header
			OR (is_array($prev) AND in_array($prev[0], $skip_prev))
			OR $next === '('
			OR (($next === '&' OR (is_array($next) AND defined('T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG')
				AND $next[0] === T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG))
				AND in_array($prev, array('(', ','), true))                         // Type &$param
			OR (is_array($next) AND in_array($next[0], array(T_DOUBLE_COLON, T_VARIABLE, T_ELLIPSIS, T_NS_SEPARATOR)))
			OR ($next === ':' AND in_array($prev, array(';', '{', '}'), true))   // goto label
			OR ($prev === '?' AND is_array($next) AND $next[0] === T_VARIABLE)  // ?Type $x
			OR in_array(strtolower($name), $keywords)
			OR isset($known[$name]) OR defined($name))
		{
			continue;
		}
		$all[$i] = array(T_CONSTANT_ENCAPSED_STRING, "'" . $name . "'", $x[2]);
		$quoted[] = $name;
	}

	if (!$quoted)
	{
		return $code;
	}
	$out = '';
	foreach ($all AS $x)
	{
		$out .= is_array($x) ? $x[1] : $x;
	}
	return $out;
}
