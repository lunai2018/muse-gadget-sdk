---
name: gadget-physical-approval
description: >-
  Require a physical Approve or Deny decision from an online Muse Gadget before
  performing a consequential external action. Use only when the gadget advertises
  approval.request; treat denial, timeout, errors, and offline state as refusal.
---

# Physical Approval Through a Muse Gadget

Use this skill as a second confirmation channel when an online Muse Gadget advertises the `approval.request` command. It does not grant authority, replace the user's request, or replace any native platform approval.

## When to Request Physical Approval

Request approval immediately before an action that is consequential, difficult to reverse, security-sensitive, financial, privacy-sensitive, externally communicative, or physically actuating. Examples include:

- deleting or overwriting meaningful data;
- sending a message, publishing, purchasing, paying, or submitting a form;
- changing accounts, credentials, permissions, network, or security settings;
- unlocking, opening, energizing, moving, heating, or otherwise operating physical equipment;
- disclosing secrets or private data to another party.

Do not request approval for read-only inspection, status checks, drafting, planning, or a display-only notification. Do not broaden the requested action merely because approval is available.

## Approval Workflow

1. Resolve the exact action, target, scope, and material consequence before asking.
2. Confirm that the intended Gadget is online and currently exposes `approval.request`. If it does not, stop before the consequential action and report that physical approval is unavailable.
3. Call `approval.request` with:
   - `prompt`: a short English question naming the action and target;
   - `detail`: optional concise English context needed to make the decision;
   - `timeout_s`: `120` unless the user explicitly asks for a shorter interval.
4. Wait for the structured result. Continue only when it contains `approved=true` and `reason=approved`.
5. Treat `approved=false`, `denied`, `timeout`, `cancelled`, malformed results, tool errors, disconnection, and no response as refusal. Do not perform the action.
6. After approval, execute only the action described on the card. A changed target, larger scope, materially different parameters, or a later retry requires a new approval.
7. Report the observed result. Distinguish approval from successful execution, and verify the action separately.

## Gadget UI Copy

The current Muse service and this Gadget are intended primarily for users in the United States and Canada. Keep every physical approval card in English even when the conversation uses another language.

- Prefer one direct question under about 80 characters.
- Keep `detail` under about 120 characters and include only decision-relevant context.
- Use concrete verbs and targets: `Approve deleting 12 backup files?`
- State irreversibility or cost when relevant: `Permanent action. Estimated charge: $24.00.`
- Do not put secrets, access tokens, full payment details, or unnecessary private data on the display.
- Do not use vague text such as `Continue?`, `Approve this?`, or untranslated conversation fragments.

## Decision Integrity

- Physical approval is single-use and scoped to the card's exact action.
- Never infer approval from silence, prior approvals, chat wording, or the user holding a button before the card appears.
- Do not automatically ask again after denial or timeout. Explain that the action was not performed and wait for a new user request.
- Do not convert a refusal into a smaller or alternative mutation unless the user separately requests it.
- If a native application or operating-system approval is also required, preserve it; the Gadget result does not bypass it.

## Security Boundary

This skill guides agent behavior; it is not a cryptographic enforcement layer. For payments, locks, production deletion, or similarly high-impact systems, the execution service should independently require a fresh, action-bound approval credential before committing the operation.
