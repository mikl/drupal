<?php
// $Id: node.admin.inc,v 1.47 2009-05-20 05:39:45 webchick Exp $

/**
 * @file
 * Content administration and module settings UI.
 */

/**
 * Menu callback; presents general node configuration options.
 */
function node_configure() {
  // Only show rebuild button if there are either 0, or 2 or more, rows
  // in the {node_access} table, or if there are modules that
  // implement hook_node_grants().
  if (db_query('SELECT COUNT(*) FROM {node_access}')->fetchField() != 1 || count(module_implements('node_grants')) > 0) {
    $status = '<p>' . t('If the site is experiencing problems with permissions to content, you may have to rebuild the permissions cache. Possible causes for permission problems are disabling modules or configuration changes to permissions. Rebuilding will remove all privileges to posts, and replace them with permissions based on the current modules and settings.') . '</p>';
    $status .= '<p>' . t('Rebuilding may take some time if there is a lot of content or complex permission settings. After rebuilding has completed posts will automatically use the new permissions.') . '</p>';

    $form['access'] = array(
      '#type' => 'fieldset',
      '#title' => t('Node access status'),
    );
    $form['access']['status'] = array('#markup' => $status);
    $form['access']['rebuild'] = array(
      '#type' => 'submit',
      '#value' => t('Rebuild permissions'),
      '#submit' => array('node_configure_access_submit'),
    );
  }

  $form['default_nodes_main'] = array(
    '#type' => 'select', '#title' => t('Number of posts on main page'),
    '#default_value' => 10,
    '#options' => drupal_map_assoc(array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30)),
    '#description' => t('The default maximum number of posts to display per page on overview pages such as the main page.')
  );
  $form['teaser_length'] = array(
    '#type' => 'select', '#title' => t('Length of trimmed posts'),
    '#default_value' => 600,
    '#options' => drupal_map_assoc(array(0, 200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000), '_node_characters'),
    '#description' => t("The maximum number of characters used in the trimmed version of a post. Drupal will use this setting to determine at which offset long posts should be trimmed. The trimmed version of a post is typically used as a teaser when displaying the post on the main page, in XML feeds, etc. To disable teasers, set to 'Unlimited' . Note that this setting will only affect new or updated content and will not affect existing teasers.")
  );

  $form['node_preview'] = array(
    '#type' => 'radios',
    '#title' => t('Preview post'),
    '#default_value' => 0,
    '#options' => array(t('Optional'), t('Required')),
    '#description' => t('Must users preview posts before submitting?'),
  );

  return system_settings_form($form, TRUE);
}

/**
 * Helper function for teaser length choices.
 */
function _node_characters($length) {
  return ($length == 0) ? t('Unlimited') : format_plural($length, '1 character', '@count characters');
}

/**
 * Form button submit callback.
 */
function node_configure_access_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/content/node-settings/rebuild';
}

/**
 * Menu callback: confirm rebuilding of permissions.
 */
function node_configure_rebuild_confirm() {
  return confirm_form(array(), t('Are you sure you want to rebuild the permissions on site content?'),
                  'admin/content/node-settings', t('This action rebuilds all permissions on site content, and may be a lengthy process. This action cannot be undone.'), t('Rebuild permissions'), t('Cancel'));
}

/**
 * Handler for wipe confirmation
 */
function node_configure_rebuild_confirm_submit($form, &$form_state) {
  node_access_rebuild(TRUE);
  $form_state['redirect'] = 'admin/content/node-settings';
}

/**
 * Implementation of hook_node_operations().
 */
function node_node_operations() {
  $operations = array(
    'publish' => array(
      'label' => t('Publish'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('status' => 1)),
    ),
    'unpublish' => array(
      'label' => t('Unpublish'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('status' => 0)),
    ),
    'promote' => array(
      'label' => t('Promote to front page'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('status' => 1, 'promote' => 1)),
    ),
    'demote' => array(
      'label' => t('Demote from front page'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('promote' => 0)),
    ),
    'sticky' => array(
      'label' => t('Make sticky'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('status' => 1, 'sticky' => 1)),
    ),
    'unsticky' => array(
      'label' => t('Remove stickiness'),
      'callback' => 'node_mass_update',
      'callback arguments' => array('updates' => array('sticky' => 0)),
    ),
    'delete' => array(
      'label' => t('Delete'),
      'callback' => NULL,
    ),
  );
  return $operations;
}

/**
 * List node administration filters that can be applied.
 */
function node_filters() {
  // Regular filters
  $filters['status'] = array(
    'title' => t('status'),
    'options' => array(
      'status-1' => t('published'),
      'status-0' => t('not published'),
      'promote-1' => t('promoted'),
      'promote-0' => t('not promoted'),
      'sticky-1' => t('sticky'),
      'sticky-0' => t('not sticky'),
    ),
  );
  // Include translation states if we have this module enabled
  if (module_exists('translation')) {
    $filters['status']['options'] += array(
      'translate-0' => t('Up to date translation'),
      'translate-1' => t('Outdated translation'),
    );
  }

  $filters['type'] = array('title' => t('type'), 'options' => node_get_types('names'));

  // The taxonomy filter
  if ($taxonomy = module_invoke('taxonomy', 'form_all', 1)) {
    $filters['term'] = array('title' => t('term'), 'options' => $taxonomy);
  }
  // Language filter if there is a list of languages
  if ($languages = module_invoke('locale', 'language_list')) {
    $languages = array('' => t('Language neutral')) + $languages;
    $filters['language'] = array('title' => t('language'), 'options' => $languages);
  }
  return $filters;
}

/**
 * Build query for node administration filters based on session.
 */
function node_build_filter_query() {
  // Build query
  $where = $args = array();
  $join = '';
  foreach ($_SESSION['node_overview_filter'] as $index => $filter) {
    list($key, $value) = $filter;
    switch ($key) {
      case 'status':
        // Note: no exploitable hole as $key/$value have already been checked when submitted
        list($key, $value) = explode('-', $value, 2);
        $where[] = 'n.' . $key . ' = %d';
        break;
      case 'term':
        $table = "tn$index";
        $where[] = "$table.tid = %d";
        $join .= "INNER JOIN {taxonomy_term_node} $table ON n.nid = $table.nid ";
        break;
      case 'type':
        $where[] = "n.type = '%s'";
        break;
      case 'language':
        $where[] = "n.language = '%s'";
        break;
    }
    $args[] = $value;
  }
  $where = count($where) ? 'WHERE ' . implode(' AND ', $where) : '';

  return array('where' => $where, 'join' => $join, 'args' => $args);
}

/**
 * Return form for node administration filters.
 */
function node_filter_form() {
  if (!isset($_SESSION['node_overview_filter'])) {
    drupal_set_session('node_overview_filter', array());
  }
  $session = &$_SESSION['node_overview_filter'];
  $filters = node_filters();

  $i = 0;
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only items where'),
    '#theme' => 'node_filters',
  );
  $form['#submit'][] = 'node_filter_form_submit';
  foreach ($session as $filter) {
    list($type, $value) = $filter;
    if ($type == 'term') {
      // Load term name from DB rather than search and parse options array.
      $value = module_invoke('taxonomy', 'term_load', $value);
      $value = $value->name;
    }
    elseif ($type == 'language') {
      $value = empty($value) ? t('Language neutral') : module_invoke('locale', 'language_name', $value);
    }
    else {
      $value = $filters[$type]['options'][$value];
    }
    if ($i++) {
      $form['filters']['current'][] = array('#markup' => t('<em>and</em> where <strong>%a</strong> is <strong>%b</strong>', array('%a' => $filters[$type]['title'], '%b' => $value)));
    }
    else {
      $form['filters']['current'][] = array('#markup' => t('<strong>%a</strong> is <strong>%b</strong>', array('%a' => $filters[$type]['title'], '%b' => $value)));
    }
    if (in_array($type, array('type', 'language'))) {
      // Remove the option if it is already being filtered on.
      unset($filters[$type]);
    }
  }

  foreach ($filters as $key => $filter) {
    $names[$key] = $filter['title'];
    $form['filters']['status'][$key] = array('#type' => 'select', '#options' => $filter['options']);
  }

  $form['filters']['filter'] = array('#type' => 'radios', '#options' => $names, '#default_value' => 'status');
  $form['filters']['buttons']['submit'] = array('#type' => 'submit', '#value' => (count($session) ? t('Refine') : t('Filter')));
  if (count($session)) {
    $form['filters']['buttons']['undo'] = array('#type' => 'submit', '#value' => t('Undo'));
    $form['filters']['buttons']['reset'] = array('#type' => 'submit', '#value' => t('Reset'));
  }

  drupal_add_js('misc/form.js');

  return $form;
}

/**
 * Theme node administration filter form.
 *
 * @ingroup themeable
 */
function theme_node_filter_form($form) {
  $output = '';
  $output .= '<div id="node-admin-filter">';
  $output .= drupal_render($form['filters']);
  $output .= '</div>';
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Theme node administration filter selector.
 *
 * @ingroup themeable
 */
function theme_node_filters($form) {
  $output = '';
  $output .= '<ul class="clearfix">';
  if (!empty($form['current'])) {
    foreach (element_children($form['current']) as $key) {
      $output .= '<li>' . drupal_render($form['current'][$key]) . '</li>';
    }
  }

  $output .= '<li><dl class="multiselect">' . (!empty($form['current']) ? '<dt><em>' . t('and') . '</em> ' . t('where') . '</dt>' : '') . '<dd class="a">';
  foreach (element_children($form['filter']) as $key) {
    $output .= drupal_render($form['filter'][$key]);
  }
  $output .= '</dd>';

  $output .= '<dt>' . t('is') . '</dt><dd class="b">';

  foreach (element_children($form['status']) as $key) {
    $output .= drupal_render($form['status'][$key]);
  }
  $output .= '</dd>';

  $output .= '</dl>';
  $output .= '<div class="container-inline" id="node-admin-buttons">' . drupal_render($form['buttons']) . '</div>';
  $output .= '</li></ul>';

  return $output;
}

/**
 * Process result from node administration filter form.
 */
function node_filter_form_submit($form, &$form_state) {
  $filters = node_filters();
  switch ($form_state['values']['op']) {
    case t('Filter'):
    case t('Refine'):
      if (isset($form_state['values']['filter'])) {
        $filter = $form_state['values']['filter'];

        // Flatten the options array to accommodate hierarchical/nested options.
        $flat_options = form_options_flatten($filters[$filter]['options']);

        if (isset($flat_options[$form_state['values'][$filter]])) {
          if (!isset($_SESSION['node_overview_filter'])) {
            drupal_set_session('node_overview_filter', array());
          }
          $_SESSION['node_overview_filter'][] = array($filter, $form_state['values'][$filter]);
        }
      }
      break;
    case t('Undo'):
      array_pop($_SESSION['node_overview_filter']);
      break;
    case t('Reset'):
      drupal_set_session('node_overview_filter', array());
      break;
  }
}

/**
 * Make mass update of nodes, changing all nodes in the $nodes array
 * to update them with the field values in $updates.
 *
 * IMPORTANT NOTE: This function is intended to work when called
 * from a form submit handler. Calling it outside of the form submission
 * process may not work correctly.
 *
 * @param array $nodes
 *   Array of node nids to update.
 * @param array $updates
 *   Array of key/value pairs with node field names and the
 *   value to update that field to.
 */
function node_mass_update($nodes, $updates) {
  // We use batch processing to prevent timeout when updating a large number
  // of nodes.
  if (count($nodes) > 10) {
    $batch = array(
      'operations' => array(
        array('_node_mass_update_batch_process', array($nodes, $updates))
      ),
      'finished' => '_node_mass_update_batch_finished',
      'title' => t('Processing'),
      // We use a single multi-pass operation, so the default
      // 'Remaining x of y operations' message will be confusing here.
      'progress_message' => '',
      'error_message' => t('The update has encountered an error.'),
      // The operations do not live in the .module file, so we need to
      // tell the batch engine which file to load before calling them.
      'file' => drupal_get_path('module', 'node') . '/node.admin.inc',
    );
    batch_set($batch);
  }
  else {
    foreach ($nodes as $nid) {
      _node_mass_update_helper($nid, $updates);
    }
    drupal_set_message(t('The update has been performed.'));
  }
}

/**
 * Node Mass Update - helper function.
 */
function _node_mass_update_helper($nid, $updates) {
  $node = node_load($nid, NULL, TRUE);
  foreach ($updates as $name => $value) {
    $node->$name = $value;
  }
  node_save($node);
  return $node;
}

/**
 * Node Mass Update Batch operation
 */
function _node_mass_update_batch_process($nodes, $updates, &$context) {
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($nodes);
    $context['sandbox']['nodes'] = $nodes;
  }

  // Process nodes by groups of 5.
  $count = min(5, count($context['sandbox']['nodes']));
  for ($i = 1; $i <= $count; $i++) {
    // For each nid, load the node, reset the values, and save it.
    $nid = array_shift($context['sandbox']['nodes']);
    $node = _node_mass_update_helper($nid, $updates);

    // Store result for post-processing in the finished callback.
    $context['results'][] = l($node->title, 'node/' . $node->nid);

    // Update our progress information.
    $context['sandbox']['progress']++;
  }

  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Node Mass Update Batch 'finished' callback.
 */
function _node_mass_update_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(t('The update has been performed.'));
  }
  else {
    drupal_set_message(t('An error occurred and processing did not complete.'), 'error');
    $message = format_plural(count($results), '1 item successfully processed:', '@count items successfully processed:');
    $message .= theme('item_list', $results);
    drupal_set_message($message);
  }
}

/**
 * Menu callback: content administration.
 */
function node_admin_content($form_state) {
  if (isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return node_multiple_delete_confirm($form_state, array_filter($form_state['values']['nodes']));
  }
  $form = node_filter_form();

  $form['#theme'] = 'node_filter_form';
  $form['admin'] = node_admin_nodes();

  return $form;
}

/**
 * Form builder: Builds the node administration overview.
 */
function node_admin_nodes() {
  // Enable language column if translation module is enabled
  // or if we have any node with language.
  $multilanguage = (module_exists('translation') || db_query("SELECT COUNT(*) FROM {node} WHERE language != ''")->fetchField());

  // Build the sortable table header.
  $header = array();
  $header[] = theme('table_select_header_cell');
  $header[] = array('data' => t('Title'), 'field' => 'n.title');
  $header[] = array('data' => t('Type'), 'field' => 'n.type');
  $header[] = array('data' => t('Author'), 'field' => 'u.name');
  $header[] = array('data' => t('Status'), 'field' => 'n.status');
  $header[] = array('data' => t('Updated'), 'field' => 'n.changed', 'sort' => 'desc');
  if ($multilanguage) {
    $header[] = array('data' => t('Language'), 'field' => 'n.language');
  }
  $header[] = array('data' => t('Operations'));

  $form['header'] = array(
   '#type' => 'value',
   '#value' => $header,
  );

  // Build the query and load the nodes we want to display.
  $filter = node_build_filter_query();

  $sort = tablesort_sql($header, '', 'n.changed DESC');
  $result = pager_query(db_rewrite_sql('SELECT n.*, u.name FROM {node} n '. $filter['join'] .' INNER JOIN {users} u ON n.uid = u.uid '. $filter['where'] . $sort), 50, 0, NULL, $filter['args']);

  // Build the 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $options = array();
  foreach (module_invoke_all('node_operations') as $operation => $array) {
    $options[$operation] = $array['label'];
  }
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'approve',
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#submit' => array('node_admin_nodes_submit'),
  );

  $languages = language_list();
  $destination = drupal_get_destination();
  $nodes = array();
  while ($node = db_fetch_object($result)) {
    $nodes[$node->nid] = '';
    $options = empty($node->language) ? array() : array('language' => $languages[$node->language]);
    $form['title'][$node->nid] = array('#markup' => l($node->title, 'node/' . $node->nid, $options) . ' ' . theme('mark', node_mark($node->nid, $node->changed)));
    $form['name'][$node->nid] =  array('#markup' => check_plain(node_get_types('name', $node)));
    $form['username'][$node->nid] = array('#markup' => theme('username', $node));
    $form['status'][$node->nid] =  array('#markup' => ($node->status ? t('published') : t('not published')));
    $form['changed'][$node->nid] = array('#markup' => format_date($node->changed, 'small'));
    if ($multilanguage) {
      $form['language'][$node->nid] = array('#markup' => empty($node->language) ? t('Language neutral') : t($languages[$node->language]->name));
    }
    $form['operations'][$node->nid] = array('#markup' => l(t('edit'), 'node/' . $node->nid . '/edit', array('query' => $destination)));
  }
  $form['nodes'] = array(
    '#type' => 'checkboxes',
    '#options' => $nodes,
  );
  $form['pager'] = array('#markup' => theme('pager', NULL));
  $form['#theme'] = 'node_admin_nodes';
  return $form;
}

/**
 * Validate node_admin_nodes form submissions.
 *
 * Check if any nodes have been selected to perform the chosen
 * 'Update option' on.
 */
function node_admin_nodes_validate($form, &$form_state) {
  $nodes = array_filter($form_state['values']['nodes']);
  if (count($nodes) == 0) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Process node_admin_nodes form submissions.
 *
 * Execute the chosen 'Update option' on the selected nodes.
 */
function node_admin_nodes_submit($form, &$form_state) {
  $operations = module_invoke_all('node_operations');
  $operation = $operations[$form_state['values']['operation']];
  // Filter out unchecked nodes
  $nodes = array_filter($form_state['values']['nodes']);
  if ($function = $operation['callback']) {
    // Add in callback arguments if present.
    if (isset($operation['callback arguments'])) {
      $args = array_merge(array($nodes), $operation['callback arguments']);
    }
    else {
      $args = array($nodes);
    }
    call_user_func_array($function, $args);

    cache_clear_all();
  }
  else {
    // We need to rebuild the form to go to a second step. For example, to
    // show the confirmation form for the deletion of nodes.
    $form_state['rebuild'] = TRUE;
  }
}


/**
 * Theme node administration overview.
 *
 * @ingroup themeable
 */
function theme_node_admin_nodes($form) {
  $output = '';
  $output .= drupal_render($form['options']);

  $header = $form['header']['#value'];

  $has_posts = isset($form['title']) && is_array($form['title']);
  if ($has_posts) {
    $rows = array();
    foreach (element_children($form['title']) as $key) {
      $row = array();
      $row[] = drupal_render($form['nodes'][$key]);
      $row[] = drupal_render($form['title'][$key]);
      $row[] = drupal_render($form['name'][$key]);
      $row[] = drupal_render($form['username'][$key]);
      $row[] = drupal_render($form['status'][$key]);
      $row[] = drupal_render($form['changed'][$key]);
      if (isset($form['language'])) {
        $row[] = drupal_render($form['language'][$key]);
      }
      $row[] = drupal_render($form['operations'][$key]);
      $rows[] = $row;
    }
  }
  else {
    $rows[] = array(
      array('data' => t('No content available.'), 'colspan' => count($header)),
    );
  }

  $output .= theme('table', $header, $rows);

  if ($form['pager']['#markup']) {
    $output .= drupal_render($form['pager']);
  }

  $output .= drupal_render_children($form);

  return $output;
}

function node_multiple_delete_confirm(&$form_state, $nodes) {
  $form['nodes'] = array('#prefix' => '<ul>', '#suffix' => '</ul>', '#tree' => TRUE);
  // array_filter returns only elements with TRUE values
  foreach ($nodes as $nid => $value) {
    $title = db_query('SELECT title FROM {node} WHERE nid = :nid', array(':nid' => $nid))->fetchField();
    $form['nodes'][$nid] = array(
      '#type' => 'hidden',
      '#value' => $nid,
      '#prefix' => '<li>',
      '#suffix' => check_plain($title) . "</li>\n",
    );
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');
  $form['#submit'][] = 'node_multiple_delete_confirm_submit';
  $confirm_question = format_plural(count($nodes), 
                                  'Are you sure you want to delete this item?', 
                                  'Are you sure you want to delete these items?');
  return confirm_form($form, 
                    $confirm_question,
                    'admin/content/node', t('This action cannot be undone.'),
                    t('Delete'), t('Cancel'));
}

function node_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    node_delete_multiple(array_keys($form_state['values']['nodes']));
    $count = count($form_state['values']['nodes']);
    watchdog('content', 'Deleted @count posts.', array('@count' => $count));
    drupal_set_message(t('Deleted @count posts.', array('@count' => $count)));
  }
  $form_state['redirect'] = 'admin/content/node';
  return;
}