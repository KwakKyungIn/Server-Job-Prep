using UnityEngine;
using Protocol;

[RequireComponent(typeof(Animator))]
public class CreatureAnimator : MonoBehaviour
{
    Animator _anim;
    static readonly int MoveStateHash = Animator.StringToHash("MoveState");
    static readonly int DeadHash = Animator.StringToHash("Dead");
    static readonly int AttackHash = Animator.StringToHash("Attack");
    static readonly int HitHash = Animator.StringToHash("Hit");

    bool _dead = false;

    void Awake() => _anim = GetComponent<Animator>();

    public void SetMoveState(MoveState state)
    {
        if (_dead) return;
        _anim.SetInteger(MoveStateHash, (int)state);
    }

    public void PlayAttack()
    {
        if (_dead) return;
        _anim.SetTrigger(AttackHash);
    }

    public void PlayHit()
    {
        if (_dead) return;
        _anim.SetTrigger(HitHash);
    }

    public void SetDead()
    {
        if (_dead) return;
        _dead = true;
        _anim.SetBool(DeadHash, true);
    }
}
