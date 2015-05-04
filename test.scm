

;; curried function
(define make-adder 
  (lambda (x)
    (lambda (y) (+ x y))))

((make-adder 4) 1)


;; restargs lambda


(define begin
  (lambda (body)
    (list 'lambda '() body)))
(macroify begin)

(begin 5 6 7)

(define x (lambda () 5))

(macroify x)

(x)

(define if 
  (lambda (_test _t _f)
    (list 'or (list 'and _test _t) _f)))
(macroify if)

(if #t 5 6)

(if #f 6 5)


