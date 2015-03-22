<?php

/**
 * @file
 * Defines the base inline entity form controller.
 */

class EntityInlineEntityFormController {

  protected $entityType;
  public $settings;

  public function __construct($entityType, array $settings) {
    $this->entityType = $entityType;
    $this->settings = $settings + $this->defaultSettings();
  }

  /**
   * Returns an array of css filepaths for the current entity type, keyed
   * by theme name.
   *
   * If provided, the "base" CSS file is included for all themes.
   * If a CSS file matching the current theme exists, it will also be included.
   *
   * @code
   * return array(
   *   'base' => drupal_get_path('module', 'test_module') . '/css/inline_entity_form.base.css',
   *   'seven' => drupal_get_path('module', 'test_module') . '/css/inline_entity_form.seven.css',
   * );
   * @endcode
   */
  public function css() {
    return array();
  }

  /**
   * Returns the default entity type labels.
   */
  public function defaultLabels() {
    $labels = array(
      'singular' => t('entity'),
      'plural' => t('entities'),
    );

    $info = entity_get_info($this->entityType);
    // Commerce and its contribs declare permission labels that can be used
    // for more precise and user-friendly strings.
    if (!empty($info['permission labels'])) {
      $labels = $info['permission labels'];
    }

    return $labels;
  }

  /**
   * Returns an array of entity type labels fit for display in the UI.
   */
  public function labels() {
    $labels = $this->defaultLabels();
    // The admin has specified the exact labels that should be used.
    if ($this->settings['override_labels']) {
      $labels = array(
        'singular' => $this->settings['label_singular'],
        'plural' => $this->settings['label_plural'],
      );
    }

    return $labels;
  }

  /**
   * Returns an array of fields used to represent an entity in the IEF table.
   *
   * The fields can be either Field API fields or properties defined through
   * hook_entity_property_info().
   *
   * Modules can alter the output of this method through
   * hook_inline_entity_form_table_fields_alter().
   *
   * @param $bundles
   *   An array of allowed bundles for this widget.
   *
   * @return
   *   An array of field information, keyed by field name. Allowed keys:
   *   - type: 'field' or 'property',
   *   - label: Human readable name of the field, shown to the user.
   *   - weight: The position of the field relative to other fields.
   *   Special keys for type 'field', all optional:
   *   - formatter: The formatter used to display the field, or "hidden".
   *   - settings: An array passed to the formatter. If empty, defaults are used.
   *   - delta: If provided, limits the field to just the specified delta.
   */
  public function tableFields($bundles) {
    $info = entity_get_info($this->entityType);
    $metadata = entity_get_property_info($this->entityType);

    $fields = array();
    if (!empty($info['entity keys']['label'])) {
      $label_key = $info['entity keys']['label'];
      $fields[$label_key] = array(
        'type' => 'property',
        'label' => $metadata ? $metadata['properties'][$label_key]['label'] : t('Label'),
        'weight' => 1,
      );
    }
    else {
      $id_key = $info['entity keys']['id'];
      $fields[$id_key] = array(
        'type' => 'property',
        'label' => $metadata ? $metadata['properties'][$id_key]['label'] : t('ID'),
        'weight' => 1,
      );
    }
    if (count($bundles) > 1) {
      $bundle_key = $info['entity keys']['bundle'];
      $fields[$bundle_key] = array(
        'type' => 'property',
        'label' => $metadata ? $metadata['properties'][$bundle_key]['label'] : t('Type'),
        'weight' => 2,
      );
    }

    return $fields;
  }

  /**
   * Returns a setting value.
   *
   * @param $name
   *   The name of the setting value to return.
   *
   * @return
   *   A setting value.
   */
  public function getSetting($name) {
    return $this->settings[$name];
  }

  /**
   * Returns an array of default settings in the form of key => value.
   */
  public function defaultSettings() {
    $defaults = array();
    $defaults['allow_new'] = TRUE;
    $defaults['allow_existing'] = FALSE;
    $defaults['match_operator'] = 'CONTAINS';
    $defaults['delete_references'] = FALSE;
    $labels = $this->defaultLabels();
    $defaults['override_labels'] = FALSE;
    $defaults['label_singular'] = $labels['singular'];
    $defaults['label_plural'] = $labels['plural'];

    return $defaults;
  }

  /**
   * Returns the settings form for the current entity type.
   *
   * The settings form is embedded into the IEF widget settings form.
   * Settings are later injected into the controller through $this->settings.
   *
   * @param $field
   *   The definition of the reference field used by IEF.
   * @param $instance
   *   The definition of the reference field instance.
   */
  public function settingsForm($field, $instance) {
    $labels = $this->labels();
    $states_prefix = 'instance[widget][settings][type_settings]';

    $form = array();
    $form['allow_new'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow users to add new @label.', array('@label' => $labels['plural'])),
      '#default_value' => $this->settings['allow_new'],
    );
    $form['allow_existing'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow users to add existing @label.', array('@label' => $labels['plural'])),
      '#default_value' => $this->settings['allow_existing'],
    );
    $form['match_operator'] = array(
      '#type' => 'select',
      '#title' => t('Autocomplete matching'),
      '#default_value' => $this->settings['match_operator'],
      '#options' => array(
        'STARTS_WITH' => t('Starts with'),
        'CONTAINS' => t('Contains'),
      ),
      '#description' => t('Select the method used to collect autocomplete suggestions. Note that <em>Contains</em> can cause performance issues on sites with thousands of nodes.'),
      '#states' => array(
        'visible' => array(
          ':input[name="' . $states_prefix . '[allow_existing]"]' => array('checked' => TRUE),
        ),
      ),
    );
    // The single widget doesn't offer autocomplete functionality.
    if ($instance['widget']['type'] == 'inline_entity_form_single') {
      $form['allow_existing']['#access'] = FALSE;
      $form['match_operator']['#access'] = FALSE;
    }

    $form['delete_references'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete referenced @label when the parent entity is deleted.', array('@label' => $labels['plural'])),
      '#default_value' => $this->settings['delete_references'],
    );

    $form['override_labels'] = array(
      '#type' => 'checkbox',
      '#title' => t('Override labels'),
      '#default_value' => $this->settings['override_labels'],
    );
    $form['label_singular'] = array(
      '#type' => 'textfield',
      '#title' => t('Singular label'),
      '#default_value' => $this->settings['label_singular'],
      '#states' => array(
        'visible' => array(
          ':input[name="' . $states_prefix . '[override_labels]"]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['label_plural'] = array(
      '#type' => 'textfield',
      '#title' => t('Plural label'),
      '#default_value' => $this->settings['label_plural'],
      '#states' => array(
        'visible' => array(
          ':input[name="' . $states_prefix . '[override_labels]"]' => array('checked' => TRUE),
        ),
      ),
    );

    return $form;
  }

  /**
   * Returns the entity type managed by this controller.
   *
   * @return
   *   The entity type.
   */
  public function entityType() {
    return $this->entityType;
  }

  /**
   * Returns the entity form to be shown through the IEF widget.
   *
   * When adding data to $form_state it should be noted that there can be
   * several IEF widgets on one master form, each with several form rows,
   * leading to possible key collisions if the keys are not prefixed with
   * $entity_form['#parents'].
   *
   * @param $entity_form
   *   The entity form.
   * @param $form_state
   *   The form state of the parent form.
   */
  public function entityForm($entity_form, &$form_state) {
    $info = entity_get_info($this->entityType);
    $entity = $entity_form['#entity'];

    if (!empty($info['fieldable'])) {
      $langcode = entity_language($this->entityType, $entity);
      field_attach_form($this->entityType, $entity, $entity_form, $form_state, $langcode);
    }

    return $entity_form;
  }

  /**
   * Validates the entity form.
   *
   * @param $entity_form
   *   The entity form.
   * @param $form_state
   *   The form state of the parent form.
   */
  public function entityFormValidate($entity_form, &$form_state) {
    $info = entity_get_info($this->entityType);
    $entity = $entity_form['#entity'];

    if (!empty($info['fieldable'])) {
      field_attach_form_validate($this->entityType, $entity, $entity_form, $form_state);
    }
  }

  /**
   * Handles the submission of an entity form.
   *
   * Prepares the entity stored in $entity_form['#entity'] for saving by copying
   * the values from the form to matching properties and, if the entity is
   * fieldable, invoking Field API submit.
   *
   * @param $entity_form
   *   The entity form.
   * @param $form_state
   *   The form state of the parent form.
   */
  public function entityFormSubmit(&$entity_form, &$form_state) {
    $info = entity_get_info($this->entityType);
    list(, , $bundle) = entity_extract_ids($this->entityType, $entity_form['#entity']);
    $entity = $entity_form['#entity'];
    $entity_values = drupal_array_get_nested_value($form_state['values'], $entity_form['#parents']);

    // Copy top-level form values that are not for fields to entity properties,
    // without changing existing entity properties that are not being edited by
    // this form. Copying field values must be done using field_attach_submit().
    $values_excluding_fields = $info['fieldable'] ? array_diff_key($entity_values, field_info_instances($this->entityType, $bundle)) : $entity_values;
    foreach ($values_excluding_fields as $key => $value) {
      $entity->$key = $value;
    }

    // If entity_translation module is used to manage translations, it can
    // change original source language of the entity. Avoid this.
    /* @see EntityTranslationDefaultHandler::entityFormSubmit() */
    $form_state['entity_translation']['is_translation'] = TRUE;

    if ($info['fieldable']) {
      field_attach_submit($this->entityType, $entity, $entity_form, $form_state);
    }
  }

  /**
   * Returns the remove form to be shown through the IEF widget.
   *
   * @param $remove_form
   *   The remove form.
   * @param $form_state
   *   The form state of the parent form.
   */
  public function removeForm($remove_form, &$form_state) {
    $entity = $remove_form['#entity'];
    list($entity_id) = entity_extract_ids($this->entityType, $entity);
    $entity_label = entity_label($this->entityType, $entity);

    $remove_form['message'] = array(
      '#markup' => '<div>' . t('Are you sure you want to remove %label?', array('%label' => $entity_label)) . '</div>',
    );
    if (!empty($entity_id) && $this->getSetting('allow_existing')) {
      $access = entity_access('delete', $this->entityType, $entity);
      if ($access) {
        $labels = $this->labels();
        $remove_form['delete'] = array(
          '#type' => 'checkbox',
          '#title' => t('Delete this @type_singular from the system.', array('@type_singular' => $labels['singular'])),
        );
      }
    }

    return $remove_form;
  }

  /**
   * Handles the submission of a remove form.
   * Decides what should happen to the entity after the removal confirmation.
   *
   * @param $remove_form
   *   The remove form.
   * @param $form_state
   *   The form state of the parent form.
   *
   * @return
   *   IEF_ENTITY_UNLINK or IEF_ENTITY_UNLINK_DELETE.
   */
  public function removeFormSubmit($remove_form, &$form_state) {
    $entity = $remove_form['#entity'];
    list($entity_id) = entity_extract_ids($this->entityType, $entity);
    $form_values = drupal_array_get_nested_value($form_state['values'], $remove_form['#parents']);
    // This entity hasn't been saved yet, we can just unlink it.
    if (empty($entity_id)) {
      return IEF_ENTITY_UNLINK;
    }
    // If existing entities can be referenced, the delete happens only when
    // specifically requested (the "Permanently delete" checkbox).
    if ($this->getSetting('allow_existing') && empty($form_values['delete'])) {
      return IEF_ENTITY_UNLINK;
    }

    return IEF_ENTITY_UNLINK_DELETE;
  }

  /**
   * Permanently saves the given entity.
   *
   * @param $entity
   *   The entity to save.
   * @param array $context
   *   Available keys:
   *   - parent_entity_type: The type of the parent entity.
   *   - parent_entity: The parent entity.
   */
  public function save($entity, $context) {
    entity_save($this->entityType, $entity);
  }

  /**
   * Delete permanently saved entities.
   *
   * @param $ids
   *   An array of entity IDs.
   * @param array $context
   *   Available keys:
   *   - parent_entity_type: The type of the parent entity.
   *   - parent_entity: The parent entity.
   */
  public function delete($ids, $context) {
    entity_delete_multiple($this->entityType, $ids);
  }

  /**
   * Processes entity translations.
   *
   * @param object $entity
   *   The entity object.
   * @param array $context
   *   Available keys:
   *   - parent_entity_type: The type of the parent entity.
   *   - parent_entity: The parent entity.
   *
   * @return bool
   *   Returns TRUE if there were some changes made to the entity translations,
   *   and therefore it should be saved.
   */
  public function processTranslations($entity, $context) {
    if (!module_exists('entity_translation')) {
      return FALSE;
    }
    if (!entity_translation_enabled($context['parent_entity_type'], $context['parent_entity']) || !entity_translation_enabled($this->entityType, $entity)) {
      return FALSE;
    }

    $parent_handler = entity_translation_get_handler($context['parent_entity_type'], $context['parent_entity']);
    $parent_langcode = $parent_handler->getFormLanguage();
    if ($parent_langcode == LANGUAGE_NONE) {
      return FALSE;
    }

    $handler = entity_translation_get_handler($this->entityType, $entity);
    $langcode = $handler->getFormLanguage();
    if ($langcode == $parent_langcode) {
      return FALSE;
    }

    $source_langcode = $langcode;
    $target_langcode = $parent_langcode;

    if ($langcode == LANGUAGE_NONE) {
      // Initialize translations.
      $entity_info = entity_get_info($this->entityType);
      if (isset($entity_info['entity keys']['language'])) {
        $entity->{$entity_info['entity keys']['language']} = $target_langcode;
      }
      $handler->setTranslation(array(
        'status' => 1,
        'language' => $target_langcode,
      ));
      $handler->setOriginalLanguage($target_langcode);
    }
    else {
      // Create/update translation record.
      $translations = $handler->getTranslations();
      $handler->setTranslation(array(
        'status' => 1,
        'language' => $target_langcode,
        'source' => ($translations->original == $target_langcode) ? '' : $translations->original,
      ));
    }

    // Process fields.
    if ($source_langcode == LANGUAGE_NONE) {
      $this->processFieldTranslations($entity, $source_langcode, $target_langcode);
    }
    else {
      list($entity_id) = entity_extract_ids($this->entityType, $entity);
      if (empty($entity_id)) {
        $this->processFieldTranslations($entity, $source_langcode, $target_langcode);
      }
      else {
        $entity_unchanged = entity_load_unchanged($this->entityType, $entity_id);
        $this->processFieldTranslations($entity, $source_langcode, $target_langcode, $entity_unchanged);
      }
    }

    return TRUE;
  }

  /**
   * Processes fields translations.
   *
   * @param object $entity
   * @param string $source_langcode
   * @param string $target_langcode
   * @param object|null $entity_unchanged
   *   If the unchanged entity is provided, the source values will be restored
   *   from it. If it's omitted, the source values will be removed.
   */
  protected function processFieldTranslations($entity, $source_langcode, $target_langcode, $entity_unchanged = NULL) {
    list(, , $bundle) = entity_extract_ids($this->entityType, $entity);

    // Prepare fields info. The cache only contains information about
    // translatable fields. Its structure:
    // [
    //   {entity type} => [
    //     {bundle} => [
    //       0 => {field name},
    //       1 => {title module legacy field name or null},
    //       2 => {title module field replacement info or null},
    //     ],
    //     ...,
    //   ],
    //   ...,
    // ]
    $cache =& drupal_static(__CLASS__ . '::' . __METHOD__, array());
    if (!isset($cache[$this->entityType][$bundle])) {
      $cache[$this->entityType][$bundle] = array();
      $replacement_info = module_invoke('title', 'field_replacement_info', $this->entityType);
      $fields = array_intersect_key(field_info_fields(), field_info_instances($this->entityType, $bundle));
      foreach ($fields as $field_name => $field_info) {
        if ($field_info['translatable']) {
          $legacy_field = NULL;
          $legacy_field_info = NULL;
          if (!empty($replacement_info)) {
            foreach ($replacement_info as $_legacy_field => $_legacy_field_info) {
              if ($_legacy_field_info['field']['field_name'] == $field_name) {
                $legacy_field = $_legacy_field;
                $legacy_field_info = $_legacy_field_info;
                break;
              }
            }
          }
          $cache[$this->entityType][$bundle][] = array($field_name, $legacy_field, $legacy_field_info);
        }
      }
    }

    // Move or copy field values to the target language.
    foreach ($cache[$this->entityType][$bundle] as $field_info) {
      list($field_name, $legacy_field, $legacy_field_info) = $field_info;

      // Copy source value to target language.
      if (isset($entity->{$field_name}[$source_langcode])) {
        $entity->{$field_name}[$target_langcode] = $entity->{$field_name}[$source_langcode];
      }
      else {
        unset($entity->{$field_name}[$target_langcode]);
      }

      // Restore source value from unchanged entity, or remove it.
      if (isset($entity_unchanged->{$field_name}[$source_langcode])) {
        $entity->{$field_name}[$source_langcode] = $entity_unchanged->{$field_name}[$source_langcode];
      }
      else {
        unset($entity->{$field_name}[$source_langcode]);
      }

      // The title module will call title_field_sync_set() on entity save. To
      // make it use correct values, call title_field_sync_get() first.
      if (isset($legacy_field)) {
        module_invoke('title', 'field_sync_get', $this->entityType, $entity, $legacy_field, $legacy_field_info);
      }
    }
  }

}
