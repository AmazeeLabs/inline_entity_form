<?php

/**
 * @file
 * Defines the inline entity form controller for Nodes.
 */

class NodeInlineEntityFormController extends EntityInlineEntityFormController {

  /**
  * Overrides EntityInlineEntityFormController::defaultLabels().
   */
  public function defaultLabels() {
    $labels = array(
      'singular' => t('node'),
      'plural' => t('nodes'),
    );
    return $labels;
  }

  /**
   * Overrides EntityInlineEntityFormController::tableFields().
   */
  public function tableFields($bundles) {
    $fields = parent::tableFields($bundles);
    $fields['status'] = array(
      'type' => 'property',
      'label' => t('Status'),
      'weight' => 100,
    );

    return $fields;
  }

  /**
   * Overrides EntityInlineEntityFormController::entityForm().
   */
  public function entityForm($entity_form, &$form_state) {
    $node = $entity_form['#entity'];
    $type = node_type_get_type($node);
    $extra_fields = field_info_extra_fields('node', $node->type, 'form');

    // Do some prep work on the node, similarly to node_form().
    if (!isset($node->title)) {
      $node->title = NULL;
    }
    node_object_prepare($node);

    $entity_form['title'] = array(
      '#type' => 'textfield',
      '#title' => check_plain($type->title_label),
      '#required' => TRUE,
      '#default_value' => $node->title,
      '#maxlength' => 255,
      // The label might be missing if the Title module has replaced it.
      '#weight' => !empty($extra_fields['title']) ? $extra_fields['title']['weight'] : -5,
    );
    $entity_form['status'] = array(
      '#type' => 'radios',
      '#access' => user_access('administer nodes'),
      '#title' => t('Status'),
      '#default_value' => $node->status,
      '#options' => array(1 => t('Published'), 0 => t('Unpublished')),
      '#required' => TRUE,
      '#weight' => 99,
    );


    $langcode = entity_language('node', $node);
    if (($handler = module_invoke('entity_translation', 'get_handler', $this->entityType, $entity_form['#entity'])) && !$handler->isNewEntity()) {
      $langcode = !empty($entity_form['#parent_language']) ? $entity_form['#parent_language'] : $handler->getLanguage();
    }

    field_attach_form('node', $node, $entity_form, $form_state, $langcode);

    // Make sure the entity_translation_prepare_element() will have the correct
    // value in the '#source', which is the language of the parent entity.
    // @todo: This needs a lot of testing: creating new nodes, update, with or
    // without translation, etc...
    list(, , $bundle) = entity_extract_ids($this->entityType, $entity_form['#entity']);
    foreach (field_info_instances($this->entityType, $bundle) as $instance) {
      $field_name = $instance['field_name'];
      $field = field_info_field($field_name);

      if ($field['translatable'] && !empty($entity_form[$field_name])) {
        $element = &$entity_form[$field_name];
        $element['#source'] = $langcode;
      }
    }

    return $entity_form;
  }

  /**
   * Overrides EntityInlineEntityFormController::entityFormSubmit().
   */
  public function entityFormSubmit(&$entity_form, &$form_state) {
    parent::entityFormSubmit($entity_form, $form_state);

    node_submit($entity_form['#entity']);
    $child_form_state = form_state_defaults();
    $child_form_state['values'] = drupal_array_get_nested_value($form_state['values'], $entity_form['#parents']);
    foreach (module_implements('node_submit') as $module) {
      $function = $module . '_node_submit';
      $function($entity_form['#entity'], $entity_form, $child_form_state);
    }
  }
}
