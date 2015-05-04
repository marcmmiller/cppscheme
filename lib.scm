
;;
;; Scheme minimal library that assumes a scheme interpreter
;; with the following features:
;;
;; Special forms:
;;  lambda (with "rest" support)
;;  and
;;  or
;;  (define x <something>) form
;;  macroify
;;
;; Builtin functions:
;;  cons
;;  eq?
;;  null?
;;
(define begin
  (lambda (stm . rest)
    (cons (cons 'lambda
                (cons '()
                      (cons stm
                            rest))) '())))
(macroify begin)

(define list
  (lambda (first . rest)
    (cons first rest)))

(define if 
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))
(macroify if)

(define not
  (lambda (x) (if x #f #t)))

(define one-map
  (lambda (func lst)
    (if (null? lst)
        '()
        (cons (func (car list))
              (one-map (cdr list))))))
;;
;; need 'apply' for this to work
;; (define map
;;   (lambda (func . lst)
;;     (if (null? (car lst))
;;         '()
;;         (cons (apply func (one-map car lst))
;;               (apply map func (one-map cdr lst))))))

