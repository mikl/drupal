<?php
// $Id: profile.pages.inc,v 1.15 2009-05-16 15:23:16 webchick Exp $

/**
 * @file
 * User page callbacks for the profile module.
 */

/**
 * Menu callback; display a list of user information.
 */
function profile_browse() {
  // Ensure that the path is converted to 3 levels always.
  list(, $name, $value) = array_pad(explode('/', $_GET['q'], 3), 3, '');

  $field = db_fetch_object(db_query("SELECT DISTINCT(fid), type, title, page, visibility FROM {profile_field} WHERE name = '%s'", $name));

  if ($name && $field->fid) {
    // Only allow browsing of fields that have a page title set.
    if (empty($field->page)) {
      drupal_not_found();
      return;
    }
    // Do not allow browsing of private and hidden fields by non-admins.
    if (!user_access('administer users') && ($field->visibility == PROFILE_PRIVATE || $field->visibility == PROFILE_HIDDEN)) {
      drupal_access_denied();
      return;
    }

    // Compile a list of fields to show.
    $fields = array();
    $result = db_query('SELECT name, title, type, weight, page FROM {profile_field} WHERE fid != %d AND visibility = %d ORDER BY weight', $field->fid, PROFILE_PUBLIC_LISTINGS);
    while ($record = db_fetch_object($result)) {
      $fields[] = $record;
    }

    // Determine what query to use:
    $arguments = array($field->fid);
    switch ($field->type) {
      case 'checkbox':
        $query = 'v.value = 1';
        break;
      case 'textfield':
      case 'selection':
        $query = "v.value = '%s'";
        $arguments[] = $value;
        break;
      case 'list':
        $query = "v.value LIKE '%%%s%%'";
        $arguments[] = $value;
        break;
      default:
        drupal_not_found();
        return;
    }

    // Extract the affected users:
    $result = pager_query("SELECT u.uid, u.access FROM {users} u INNER JOIN {profile_value} v ON u.uid = v.uid WHERE v.fid = %d AND $query AND u.access != 0 AND u.status != 0 ORDER BY u.access DESC", 20, 0, NULL, $arguments)->fetchAllAssoc('uid');

    // Load the users.
    $users = user_load_multiple(array_keys($result));

    $content = '';
    foreach ($users as $account) {
      $profile = _profile_update_user_fields($fields, $account);
      $content .= theme('profile_listing', $account, $profile);
    }
    $output = theme('profile_wrapper', $content);
    $output .= theme('pager', NULL);

    if ($field->type == 'selection' || $field->type == 'list' || $field->type == 'textfield') {
      $title = strtr(check_plain($field->page), array('%value' => theme('placeholder', $value)));
    }
    else {
      $title = check_plain($field->page);
    }

    drupal_set_title($title, PASS_THROUGH);
    return $output;
  }
  elseif ($name && !$field->fid) {
    drupal_not_found();
  }
  else {
    // Compile a list of fields to show.
    $fields = array();
    $result = db_query('SELECT name, title, type, weight, page, visibility FROM {profile_field} WHERE visibility = %d ORDER BY category, weight', PROFILE_PUBLIC_LISTINGS);
    while ($record = db_fetch_object($result)) {
      $fields[] = $record;
    }

    // Extract the affected users:
    $result = pager_query('SELECT uid, access FROM {users} WHERE uid > 0 AND status != 0 AND access != 0 ORDER BY access DESC', 20, 0, NULL)->fetchAllAssoc('uid');
    $users = user_load_multiple(array_keys($result));
    $content = '';
    foreach ($users as $account) {
      $profile = _profile_update_user_fields($fields, $account);
      $content .= theme('profile_listing', $account, $profile);
    }
    $output = theme('profile_wrapper', $content);
    $output .= theme('pager', NULL);

    drupal_set_title(t('User list'), PASS_THROUGH);
    return $output;
  }
}

/**
 * Callback to allow autocomplete of profile text fields.
 */
function profile_autocomplete($field, $string) {
  $matches = array();
  $autocomplete_field = (bool) db_query_range("SELECT 1 FROM {profile_field} WHERE fid = :fid AND autocomplete = 1", array(':fid' => $field), 0, 1)->fetchField();
  if ($autocomplete_field) {
    $result = db_query_range("SELECT value FROM {profile_value} WHERE fid = :fid AND LOWER(value) LIKE LOWER(:value) GROUP BY value ORDER BY value ASC", array(
      ':fid' => $field,
      ':value' => $string .'%',
    ), 0, 10);
    while ($data = db_fetch_object($result)) {
      $matches[$data->value] = check_plain($data->value);
    }
  }

  drupal_json($matches);
}