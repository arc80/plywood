// Page elements
var sidebar;
var article;

var highlighted = null;
var selected = null;
var pageCache = [];

function highlight(elementID) {
    if (highlighted) {
        highlighted.classList.remove("highlighted");
    }
    highlighted = document.getElementById(elementID);
    if (highlighted) {
        highlighted = highlighted.parentElement;
        if (highlighted) {
            highlighted.classList.add("highlighted");
        }
    }
}

function savePageState() {
    stateData = {
        path: location.pathname + location.hash,
        pageYOffset: window.pageYOffset
    };
    window.history.replaceState(stateData, null);
}

function expandToItem(targetItem) {
    var parent = targetItem.parentElement;
    if (parent.tagName == "A") {
        parent = parent.parentElement;
    }
    while (parent.tagName == "UL") {
        parent.classList.add("active");
        var li = parent.previousElementSibling;
        if (li) {
            if (li.tagName == "A") {
                li = li.children[0];
            }
            li.classList.add("caret-down");
        }                        
        parent = parent.parentElement;
    }
}

activeScrollers = [];

function updateScrollers(timestamp) {    
    for (var i = 0; i < activeScrollers.length; i++) {
        props = activeScrollers[i];
        if (props.start === null) {
            props.start = timestamp;
        } else {
            var f = (timestamp - props.start) / 100;
            if (f < 1) {
                props.element.scrollTop = props.scrollTo * f + props.scrollFrom * (1 - f);
            } else {
                props.element.scrollTop = props.scrollTo;
                activeScrollers.splice(i, 1);
                i--;
            }
        }
    }
    if (activeScrollers.length > 0) {
        requestAnimationFrame(updateScrollers);
    }
}

function smoothScrollIntoView(container, item, scrollToTop) {
    if (!item)
        return;
    var amount;
    if (scrollToTop) {        
        amount = item.getBoundingClientRect().top - 50;  // hardcoded header size        
    } else {
        amount = item.getBoundingClientRect().top - container.getBoundingClientRect().top;
        if (amount >= 0) {
            var containerBottom = Math.min(document.documentElement.clientHeight, container.getBoundingClientRect().bottom);
            amount = item.getBoundingClientRect().bottom - containerBottom;
            if (amount <= 0)
                return; // No need to scroll
        }
    }

    if (amount != 0) {       
        if (activeScrollers.length == 0) {
            requestAnimationFrame(updateScrollers);
        } 
        // Find existing item
        var props = null;
        for (var p in activeScrollers) {
            if (p.element === container) {
                props = p;
                break;
            }
        }
        if (!props) {
            props = {};
            activeScrollers.push(props)
        }
        props.element = container;
        props.start = null;
        var t = container.scrollTop;
        var maxScrollTop = container.scrollHeight - container.clientHeight;
        props.scrollTo = Math.min(t + amount, maxScrollTop);
        props.scrollFrom = Math.min(Math.max(t, props.scrollTo - 500), props.scrollTo + 500);
    }
}

function showTOCEntry(pathToMatch) {
    // Select appropriate TOC entry
    if (selected) {
        selected.classList.remove("selected");
        selected = null;
    }
    var list = sidebar.getElementsByTagName("li");
    for (var j = 0; j < list.length; j++) {
        var li = list[j];
        if (li.parentElement.getAttribute("href") == pathToMatch) {
            selected = li;
            li.classList.add("selected");
            expandToItem(li);
            smoothScrollIntoView(sidebar.firstElementChild, li, false);
        }
    }
}

var currentRequest = null;
var currentLoadingTimer = null;
var spinnerShown = false;

function navigateTo(dstPath, forward, pageYOffset) {
    if (currentRequest !== null) {
        currentRequest.abort();
        currentRequest = null;
    }

    // Update history
    if (forward) {
        history.pushState(null, null, dstPath);
    }

    // Extract anchor from dstPath
    var anchorPos = dstPath.indexOf("#");
    var anchor = (anchorPos >= 0) ? dstPath.substr(anchorPos + 1) : "";

    // Show appropriate TOC entry
    var pathToMatch = (anchorPos >= 0) ? dstPath.substr(0, anchorPos) : dstPath;
    showTOCEntry(pathToMatch);

    var applyArticle = function(responseText) {
        // Extract title
        var n = responseText.indexOf("\n");
        document.title = responseText.substr(0, n);

        // Apply article
        article.innerHTML = responseText.substr(n + 1);
        replaceLinks(article);

        // Scroll to desired position
        if (anchor != "") {
            highlight(anchor);
        }
        if (forward && anchor != "") {
            smoothScrollIntoView(document.documentElement, document.getElementById(anchor).parentElement, true);
        } else {
            window.scrollTo(0, pageYOffset);
        }
        if (location.pathname != dstPath) {
            // Should never happen
            console.log("location.pathname out of sync: '" + location.pathname + "' != '" + dstPath + "'");
        }
        savePageState();
    }
    
    // Check pageCache
    for (var i = pageCache.length - 1; i >= 0; i--) {
        var cached = pageCache[i];
        if (cached.path == pathToMatch) {
            pageCache.splice(i, 1);
            pageCache.push(cached);
            applyArticle(cached.responseText);
            return;
        }
    }

    // Not found in cache
    // Issue AJAX request for new page
    currentRequest = new XMLHttpRequest();
    currentRequest.onreadystatechange = function() {
        if (currentRequest !== this)
            return;
        if (this.readyState == 4 && this.status == 200) {
            // Completed
            currentRequest = null;
            spinnerShown = false;
            applyArticle(this.responseText);

            // Update pageCache
            pageCache.push({path: dstPath, responseText: this.responseText});
            pageCache = pageCache.slice(-20); // Limit number of saved pages
        }
    };
    currentRequest.open("GET", "/content?path=" + dstPath, true);
    currentRequest.send();            

    // Set timer to show loading spinner
    if (currentLoadingTimer !== null) {
        window.clearTimeout(currentLoadingTimer);
    }
    var showSpinnerForRequest = currentRequest;
    currentLoadingTimer = window.setTimeout(function() {
        if (spinnerShown || currentRequest !== showSpinnerForRequest)
            return;
        spinnerShown = true;
        article.innerHTML = 
'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="32px" height="32px" viewBox="0 0 100 100" style="margin: 0 auto;">\
<g>\
  <circle cx="50" cy="50" fill="none" stroke="#dbe6e8" stroke-width="12" r="36" />\
  <circle cx="50" cy="50" fill="none" stroke="#4aa5e0" stroke-width="12" r="36" stroke-dasharray="50 180" />\
  <animateTransform attributeName="transform" type="rotate" repeatCount="indefinite" dur="1s" values="0 50 50;360 50 50" \
  keyTimes="0;1" />\
</g>\
</svg>'
    }, 750);
}

function rectContains(rect, x, y) {
    return rect.left <= x && x < rect.right && rect.top <= y && y < rect.bottom;
}

function replaceLinks(root) {
    var list = root.getElementsByTagName("a");
    for (var i = 0; i < list.length; i++) {
        var a = list[i];
        var path = a.getAttribute("href");
        if (path.substr(0, 6) == "/docs/") {
            a.onclick = function(evt) {
                var caretSpan = this.querySelector(".selectable.caret span");
                if (caretSpan) {
                    var sr = caretSpan.getBoundingClientRect();
                    var inflate = 8;
                    // Hardcoding the dimensions of li.caret span::before since there's no way to retrieve them from the DOM
                    caretRect = {left: sr.left - 19 - inflate, top: sr.top + 1 - inflate, right: sr.left - 8 + inflate, bottom: sr.top + 12 + inflate};
                    if (rectContains(caretRect, evt.clientX, evt.clientY))
                        return false;
                }
                savePageState();
                navigateTo(this.getAttribute("href"), true, 0);
                cancelPopupMenu();
                return false;
            }
        }
    }
}

function onEndTransition(evt) {
    this.removeEventListener('transitionend', onEndTransition);
    this.style.removeProperty("display");
    this.style.removeProperty("transition");
    this.style.removeProperty("height");
}

function injectKeyframeAnimation() {
    var animation = '';
    var inverseAnimation = ''; 
    for (var step = 0; step <= 100; step++) {
        var f = 1 - Math.pow(1 - step / 100.0, 4);
        var xScale = 0.5 + 0.5 * f;
        var yScale = 0.01 + 0.99 * f;  
        animation += step + "% { transform: scale(" + xScale + ", " + yScale + "); }\n";
        inverseAnimation += step + "% { transform: scale(" + (1.0 / xScale) + ", " + (1.0 / yScale) + "); }\n";
    }
    var style = document.createElement('style');
    style.textContent = "@keyframes menuAnimation {\n" + animation + "}\n\n" +
        "@keyframes menuContentsAnimation {\n" + inverseAnimation + "}\n\n";
    document.head.appendChild(style);
}

//------------------------------------
// Popup menus
//------------------------------------

// {button, menu, callback}
var menuShown = null;

function cancelPopupMenu() {
    if (menuShown) {
        menuShown.menu.firstElementChild.classList.remove("expanded-content");
        menuShown.menu.classList.remove("expanded");
        document.removeEventListener("click", menuShown.callback);
        menuShown = null;
    }
}

function togglePopupMenu(button, menu) {
    var mustShow = !menuShown || (menuShown.menu != menu);
    cancelPopupMenu();
    if (mustShow) {
        menu.classList.add("expanded");
        menu.firstElementChild.classList.add("expanded-content");
        // Safari needs this to force .expanded animation to replay:
        menu.style.animation = 'none';
        void menu.offsetHeight; // triggers reflow
        menu.style.animation = '';
        // Register close hook
        menuShown = {
            button: button,
            menu: menu,
            callback: function(evt) {
                if (menuShown && menuShown.menu == menu) {
                    if (!menuShown.button.contains(evt.target) && !menuShown.menu.contains(evt.target)) {
                        cancelPopupMenu();
                    }
                }
            }
        };
        document.addEventListener("click", menuShown.callback);
    }
    return mustShow;
}

window.onload = function() { 
    injectKeyframeAnimation();
    sidebar = document.querySelector(".sidebar");
    article = document.getElementById("article");

    if ('scrollRestoration' in history) {
        history.scrollRestoration = 'manual';
    }

    highlight(location.hash.substr(1));

    var list = document.getElementsByClassName("caret");
    for (var i = 0; i < list.length; i++) {
        list[i].addEventListener("click", function() {
            var childList = this.nextElementSibling || this.parentElement.nextElementSibling;
            if (this.classList.contains("caret-down")) {
                // Collapse
                this.classList.remove("caret-down")
                childList.style.display = "block";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.remove("active");
                requestAnimationFrame(function() {
                    childList.style.transition = "height 0.15s ease-out";
                    requestAnimationFrame(function() {
                        childList.style.height = "0px";
                        childList.addEventListener('transitionend', onEndTransition);
                    });
                });
            } else {
                // Expand
                this.classList.add("caret-down");
                childList.style.display = "block";
                childList.style.transition = "height 0.15s ease-out";
                childList.style.height = childList.scrollHeight + "px";
                childList.classList.add("active");
                childList.addEventListener('transitionend', onEndTransition);
            }
        });
    }

    var getInvolvedButton = document.getElementById("get-involved");
    getInvolvedButton.addEventListener("click", function() {
        var getInvolvedMenu = document.querySelector(".get-involved-popup");
        togglePopupMenu(getInvolvedButton, getInvolvedMenu);
    });

    var threeLines = document.getElementById("three-lines");
    threeLines.addEventListener("click", function() {
        if (togglePopupMenu(threeLines, sidebar)) {
            showTOCEntry(location.pathname);
        }
    });
    threeLines.addEventListener("mouseover", function() {
        this.firstElementChild.classList.add("highlight");
    });
    threeLines.addEventListener("mouseout", function() {
        this.firstElementChild.classList.remove("highlight");
    });

    replaceLinks(sidebar);
    replaceLinks(article);
    selected = sidebar.querySelector(".selected");
}

window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}

window.onscroll = function() {
    savePageState();
}

window.addEventListener("popstate", function(evt) {
    if (evt.state) {
        cancelPopupMenu();
        navigateTo(evt.state.path, false, evt.state.pageYOffset);
    }
});
